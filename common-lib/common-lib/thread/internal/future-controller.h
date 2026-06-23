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


        const remove_type_qualifiers_t<T> &get() const;
        [[nodiscard]] remove_type_qualifiers_t<T> extract();

    private:
        void post_success();
        void post_fail();

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
            post_success();
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
            post_fail();
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
                post_fail();
            }
        }
    }

    template<typename T, typename ThreadPool>
    void future_controller<T, ThreadPool>::set_value(T &&value)
    {
        {
            std::lock_guard guard(m_mtx);
            if(m_val || m_exception) {
                throw std::logic_error("value or exeption already set");
            }

            m_val = std::move(value);
            if(m_on_success) {
                post_success();
            }
        }

        m_cv.notify_all();
    }

    template<typename T, typename ThreadPool>
    void future_controller<T, ThreadPool>::set_exception(const std::exception_ptr &e)
    {
        {
            std::lock_guard guard(m_mtx);
            if(m_val || m_exception) {
                throw std::logic_error("value or exeption already set");
            }

            m_exception = e;
            if(m_on_fail) {
                post_fail();
            }
        }

        m_cv.notify_all();
    }

    template<typename T, typename ThreadPool>
    const remove_type_qualifiers_t<T> &future_controller<T, ThreadPool>::get() const
    {
        std::unique_lock lock(m_mtx);
        m_cv.wait(lock, [this]() { return m_val || m_exception; });
        if(m_exception) {
            std::rethrow_exception(*m_exception);
        }

        return m_val.value();
    }

    template<typename T, typename ThreadPool>
    remove_type_qualifiers_t<T> future_controller<T, ThreadPool>::extract()
    {
        std::unique_lock lock(m_mtx);
        m_cv.wait(lock, [this]() { return m_val || m_exception; });
        if(m_exception) {
            std::rethrow_exception(*m_exception);
        }
        if(!m_val.has_value()) {
            throw std::logic_error("value was extracted");
        }

        return std::move(m_val.value());
    }

    template<typename T, typename ThreadPool>
    void future_controller<T, ThreadPool>::post_success()
    {
        m_thread_pool->post([s = this->shared_from_this()]() {
            s->m_on_success->call(*(std::move(s->m_val)));
        });
    }

    template<typename T, typename ThreadPool>
    void future_controller<T, ThreadPool>::post_fail()
    {
        m_thread_pool->post([s = this->shared_from_this()]() {
            s->m_on_fail((*s->m_exception));
        });
    }
}
