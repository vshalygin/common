#pragma once
#include <common-lib/mpl/function-traits.h>
#include <common-lib/mpl/type-transform.h>

#include <memory>
#include <cassert>
#include <functional>
#include <optional>
#include <mutex>
#include <condition_variable>

namespace vshalygin::cl::internal {
    //TODO void specialization for all

    template<typename T, typename ThreadPool>
    class future_controller;

    template<typename T, typename ThreadPool>
    class future_data final
    {
        friend class future_controller<T, ThreadPool>;

        explicit future_data(std::shared_ptr<future_controller<T, ThreadPool>> controller)
            : m_controller(controller)
        {}

    public:
        future_data(const future_data &) = delete;
        future_data &operator=(const future_data &) = delete;

        future_data(future_data &&) = default;
        future_data &operator=(future_data &&) = default;

        template<typename Func>
        void apply(Func &&func) const;

    private:
        std::shared_ptr<future_controller<T, ThreadPool>> m_controller;
    };

    template<typename Arg>
    class ifuture_callback
    {
    public:
        virtual ~ifuture_callback() = default;

        virtual void call(Arg &&arg) = 0;
    };

    template<typename Func>
    class future_callback
        : public ifuture_callback<remove_type_qualifiers_t<function_arg_t<0, Func>>>
    {
        static_assert(function_arg_count_v<Func> == 1);
        static_assert(std::is_same_v<function_ret_t<Func>, void>);

        using FuncArg = function_arg_t<0, Func>;

    public:
        template<typename F>
        explicit future_callback(F &&f)
            : m_func(std::forward<F>(f))
        {}

        future_callback(const future_callback &) = delete;
        future_callback &operator=(const future_callback &) = delete;

        void call(remove_type_qualifiers_t<FuncArg> &&arg) override
        {
            if constexpr(!std::is_reference_v<FuncArg>) {
                m_func(arg);
            } else {
                m_func(std::forward<FuncArg>(arg));
            }
        }

    private:
        remove_type_qualifiers_t<Func> m_func;
    };


    template<typename T, typename ThreadPool>
    class future_controller
        : public std::enable_shared_from_this<future_controller<T, ThreadPool>>
    {
        using this_type = future_controller<T, ThreadPool>;

        friend class future_data<T, ThreadPool>;

        class notify_all_on_destruct
        {
        public:
            explicit notify_all_on_destruct(std::condition_variable *cv)
                : m_cv(cv)
            {}

            notify_all_on_destruct(const notify_all_on_destruct &) = delete;
            notify_all_on_destruct &operator=(const notify_all_on_destruct &) = delete;

            ~notify_all_on_destruct()
            {
                if(m_cv) {
                    m_cv->notify_all();
                }
            }

        private:
            std::condition_variable *m_cv = nullptr;
        };

    public:
        explicit future_controller(ThreadPool *thread_pool);

        future_controller(const future_controller &) = delete;
        future_controller &operator=(const future_controller &) = delete;

        template<typename Func>
        void set_on_success(Func &&func);

        void set_on_fail(std::function<void(std::exception_ptr)> &&func);
        void set_on_fail_if_not_set(std::function<void(std::exception_ptr)> &&func);

        void set_value(T &&value);
        void set_exception(const std::exception_ptr &e);


        future_data<T, ThreadPool> get_data() const;

    private:
        void post_success(bool need_notify);
        void post_fail(bool need_notify);

        void call_success(bool need_notify);
        void call_fail(bool need_notify);

    private:
        ThreadPool *m_thread_pool;

        std::optional<remove_type_qualifiers_t<T>> m_val;
        std::optional<std::exception_ptr> m_exception;

        std::unique_ptr<ifuture_callback<remove_type_qualifiers_t<T>>> m_on_success;
        std::function<void(std::exception_ptr)> m_on_fail;
        std::function<void(std::exception_ptr)> m_on_fail_default;

        mutable std::mutex m_mtx;
        mutable std::condition_variable m_cv;
    };

    template<typename T, typename ThreadPool>
    future_controller<T, ThreadPool>::future_controller(ThreadPool *thread_pool)
        : m_thread_pool(thread_pool)
    {
        assert(m_thread_pool);
    }

    template<typename T, typename ThreadPool>
    template<typename Func>
    void future_controller<T, ThreadPool>::set_on_success(Func &&func)
    {
        std::lock_guard guard(m_mtx);
        if(m_on_success) {
            throw std::logic_error("success handler already set");
        }

        m_on_success = std::make_unique<future_callback<Func>>(std::forward<Func>(func));
        if(m_val) {
            post_success(false);
        }
    }

    template<typename T, typename ThreadPool>
    void future_controller<T, ThreadPool>::set_on_fail(
                                      std::function<void(std::exception_ptr)> &&func)
    {
        std::lock_guard guard(m_mtx);
        if(m_on_fail) {
            throw std::logic_error("fail handler already set");
        }

        m_on_fail = std::move(func);
        if(m_exception) {
            post_fail(false);
        }
    }

    template<typename T, typename ThreadPool>
    void future_controller<T, ThreadPool>::set_on_fail_if_not_set(
                                     std::function<void(std::exception_ptr)> &&func)
    {
        std::lock_guard guard(m_mtx);
        if(!m_on_fail) {
            m_on_fail = std::move(func);
            if(m_exception) {
                post_fail(false);
            }
        }
    }

    template<typename T, typename ThreadPool>
    void future_controller<T, ThreadPool>::set_value(T &&value)
    {
        {
            std::unique_lock guard(m_mtx);
            if(m_val || m_exception) {
                throw std::logic_error("value or exeption already set");
            }

            m_val = std::move(value);
            if(m_on_success) {
                post_success(true);
            } else {
                guard.unlock();
                m_cv.notify_all();
            }
        }
    }

    template<typename T, typename ThreadPool>
    void future_controller<T, ThreadPool>::set_exception(const std::exception_ptr &e)
    {
        std::unique_lock guard(m_mtx);
        if(m_val || m_exception) {
            throw std::logic_error("value or exeption already set");
        }

        m_exception = e;
        if(m_on_fail) {
            post_fail(true);
        } else {
            guard.unlock();
            m_cv.notify_all();
        }
    }

    template<typename T, typename ThreadPool>
    future_data<T, ThreadPool>
        future_controller<T, ThreadPool>::get_data() const
    {
        std::unique_lock lock(m_mtx);
        m_cv.wait(lock, [this]() { return m_val || m_exception; });
        if(m_exception) {
            std::rethrow_exception(*m_exception);
        }
        lock.unlock();

        return future_data<T, ThreadPool>(const_cast<this_type *>(this)->shared_from_this());
    }

    template<typename T, typename ThreadPool>
    void future_controller<T, ThreadPool>::post_success(bool need_notify)
    {
        m_thread_pool->post([s = this->shared_from_this(), need_notify]() {
            s->call_success(need_notify);
        });
    }

    template<typename T, typename ThreadPool>
    void future_controller<T, ThreadPool>::post_fail(bool need_notify)
    {
        m_thread_pool->post([s = this->shared_from_this(), need_notify]() {
            s->call_fail(need_notify);
        });
    }

    template<typename T, typename ThreadPool>
    void future_controller<T, ThreadPool>::call_success(bool need_notify)
    {
        notify_all_on_destruct n(need_notify ? &m_cv : nullptr);
        std::lock_guard guard(m_mtx);
        m_on_success->call(std::move(*m_val));
    }

    template<typename T, typename ThreadPool>
    void future_controller<T, ThreadPool>::call_fail(bool need_notify)
    {
        notify_all_on_destruct n(need_notify ? &m_cv : nullptr);
        std::lock_guard guard(m_mtx);
        m_on_fail(*m_exception);
    }

    template<typename T, typename ThreadPool>
    template<typename Func>
    void future_data<T, ThreadPool>::apply(Func &&func) const
    {
        static_assert(std::is_same_v<function_ret_t<Func>, void>);
        static_assert(function_arg_count_v<Func> == 1);
        static_assert(std::is_same_v<remove_type_qualifiers_t<function_arg_t<0, Func>>,
                      remove_type_qualifiers_t<T>>);

        std::lock_guard guard(m_controller->m_mtx);
        func(std::forward<function_arg_t<0, Func>>(m_controller->m_val.value()));
    }
}
