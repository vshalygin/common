#pragma once
#include "future-data.h"

#include <common-lib/mpl/type-traits.h>
#include <common-lib/mpl/type-transform.h>
#include <common-lib/synchronization/ordered-mutex.h>
#include <common-lib/synchronization/ordered-mutex-ref.h>
#include <common-lib/synchronization/ordered-lock.h>
#include <common-lib/synchronization/value-locker.h>
#include <common-lib/utils/value-proxy.h>
#include <common-lib/utils/function.h>
#include <common-lib/memory/enable-shared-from-this-manual-set.h>

#include <memory>
#include <cassert>
#include <optional>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <vector>
#include <chrono>

namespace vshalygin::cl::internal {
    class ifuture_controller
    {
    public:
        virtual ~ifuture_controller() = default;
    };

    template<typename ThreadPool, typename T>
    class future_controller
        : public enable_shared_from_this_manual_set<future_controller<ThreadPool, T>>
        , public ifuture_controller
    {
        using on_success_t = function<void(std::add_rvalue_reference_t<T>)>;
        using on_fail_t = function<void(std::exception_ptr)>;

    public:
        explicit future_controller(ThreadPool *thread_pool);

        future_controller(const future_controller &) = delete;
        future_controller &operator=(const future_controller &) = delete;

        template<typename Func>
        void set_on_success_and_fail(Func &&success_func, on_fail_t &&fail_func);

        template<typename TT = T, std::enable_if_t<std::is_void_v<TT>, int> = 0>
        void set_value();

        template<typename U, typename TT = T, std::enable_if_t<!std::is_void_v<TT>, int> = 0>
        void set_value(U &&val);

        template<typename U, typename TT = T, std::enable_if_t<!std::is_void_v<TT>, int> = 0>
        void set_value_reference(std::mutex &outer_mtx, U &&value);

        void set_exception(const std::exception_ptr &e);

        auto get() const;
        auto get();

        void wait() const;
        bool wait_for(std::chrono::milliseconds timeout) const;

        auto get_val();
        auto get_val() const;

        void add_child(std::unique_ptr<ifuture_controller> child);
        void add_dependent(std::shared_ptr<ifuture_controller> dependent);

        std::mutex &get_value_mtx_ref() const noexcept;

    private:
        using value_proxy = value_proxy<add_lvalue_ref_to_value_t<T>>;
        using on_success_mtx = ordered_mutex<0>;
        using on_fail_mtx = ordered_mutex<1>;
        using val_mtx = ordered_mutex<2>;
        using exception_mtx = ordered_mutex<3>;
        using outer_val_mtx_ref = ordered_mutex_ref<4>;

        template<typename Func>
        void set_on_success_unsafe(Func &&func);

        void set_on_fail_unsafe(on_fail_t &&func);

        void set_success(outer_val_mtx_ref outer_mtx_ref, value_proxy value);

        void wait_data_ready_or_throw() const;

        void process_on_success_async();
        void process_on_fail_async();

        void call_success(on_success_t &&func);
        void call_fail(on_fail_t &&func);

    private:
        ThreadPool *m_thread_pool;

        value_locker<std::vector<std::unique_ptr<ifuture_controller>>> m_children;
        value_locker<std::vector<std::shared_ptr<ifuture_controller>>> m_dependent;

        mutable on_success_mtx m_on_success_mtx;
        std::queue<on_success_t> m_on_success_queue;

        mutable on_fail_mtx m_on_fail_mtx;
        std::queue<on_fail_t> m_on_fail_queue;

        mutable val_mtx m_val_mtx;
        std::optional<value_proxy> m_val;

        mutable exception_mtx m_exception_mtx;
        std::optional<std::exception_ptr> m_exception;

        mutable std::condition_variable_any m_cv;

        mutable outer_val_mtx_ref m_outer_val_mtx;
    };

    template<typename ThreadPool, typename T>
    future_controller<ThreadPool, T>::future_controller(ThreadPool *thread_pool)
        : m_thread_pool(thread_pool)
    {
        assert(m_thread_pool);
    }

    template<typename ThreadPool, typename T>
    template<typename Func>
    void future_controller<ThreadPool, T>::set_on_success_and_fail(Func &&success_func, on_fail_t &&fail_func)
    {
        ordered_lock guard(m_on_success_mtx, m_on_fail_mtx);
        set_on_success_unsafe(std::move(success_func));
        set_on_fail_unsafe(std::move(fail_func));
    }

    template<typename ThreadPool, typename T>
    template<typename Func>
    void future_controller<ThreadPool, T>::set_on_success_unsafe(Func &&func)
    {
        static_assert(std::is_same_v<void, function_ret_t<Func>>,
                      "success callback return type is not void");
        if constexpr(!std::is_void_v<T>) {
            static_assert(function_arg_count_v<Func> == 1,
                          "success callback arg count is not 1");
        } else {
            static_assert(function_arg_count_v<Func> == 0,
                          "success callback arg count is not 0");
        }

        if constexpr(std::is_void_v<T>) {
            m_on_success_queue.push(std::forward<Func>(func));
        } else {
            m_on_success_queue.push([func = std::forward<Func>(func)]
                                    (std::add_rvalue_reference_t<T> val) mutable {
                func(type_qualifiers_cast<function_arg_t<0, Func>>(val));
            });
        }

        process_on_success_async();
    }

    template<typename ThreadPool, typename T>
    void future_controller<ThreadPool, T>::set_on_fail_unsafe(on_fail_t &&func)
    {
        m_on_fail_queue.push(std::move(func));

        process_on_fail_async();
    }

    template<typename ThreadPool, typename T>
    template<typename TT, std::enable_if_t<std::is_void_v<TT>, int>>
    void future_controller<ThreadPool, T>::set_value()
    {
        set_success(outer_val_mtx_ref{}, value_proxy{});
    }

    template<typename ThreadPool, typename T>
    template<typename U, typename TT, std::enable_if_t<!std::is_void_v<TT>, int>>
    void future_controller<ThreadPool, T>::set_value(U &&val)
    {
        value_proxy v;

        if constexpr(std::is_reference_v<T>) {
            v = value_proxy{ std::forward<U>(val), value_proxy_external };
        } else {
            v = value_proxy{ std::forward<U>(val), value_proxy_owned };
        }

        set_success(outer_val_mtx_ref{}, std::move(v));
    }

    template<typename ThreadPool, typename T>
    template<typename U, typename TT, std::enable_if_t<!std::is_void_v<TT>, int>>
    void future_controller<ThreadPool, T>::set_value_reference(std::mutex &outer_mtx, U &&value)
    {
        set_success(outer_val_mtx_ref{ outer_mtx },
                    value_proxy{ std::forward<U>(value), value_proxy_external });
    }

    template<typename ThreadPool, typename T>
    void future_controller<ThreadPool, T>::set_success(outer_val_mtx_ref outer_mtx_ref,
                                                       value_proxy value)
    {
        {
            ordered_lock g(m_val_mtx);

            m_val.emplace(std::move(value));
            m_outer_val_mtx = outer_mtx_ref;

            process_on_success_async();
            process_on_fail_async();
        }

        m_cv.notify_all();
    }

    template<typename ThreadPool, typename T>
    void future_controller<ThreadPool, T>::set_exception(const std::exception_ptr &e)
    {
        {
            ordered_lock g(m_exception_mtx);

            m_exception = e;

            process_on_success_async();
            process_on_fail_async();
        }

        m_cv.notify_all();
    }

    template<typename ThreadPool, typename T>
    auto future_controller<ThreadPool, T>::get() const
    {
        wait_data_ready_or_throw();
        if constexpr(!std::is_void_v<T>) {
            return future_data<T, ThreadPool>(this->shared_from_this());
        }
    }

    template<typename ThreadPool, typename T>
    auto future_controller<ThreadPool, T>::get()
    {
        wait_data_ready_or_throw();
        if constexpr(!std::is_void_v<T>) {
            return future_data<T, ThreadPool>(this->shared_from_this());
        }
    }

    template<typename ThreadPool, typename T>
    void future_controller<ThreadPool, T>::wait() const
    {
        ordered_lock lock(m_val_mtx, m_exception_mtx);
        m_cv.wait(lock, [this]() { return m_val || m_exception; });
    }

    template<typename ThreadPool, typename T>
    bool future_controller<ThreadPool, T>::wait_for(std::chrono::milliseconds timeout) const
    {
        using clock = std::chrono::steady_clock;

        const auto now = clock::now();
        const auto max_tp = clock::time_point::max();

        clock::time_point tp = (timeout > max_tp - now) ? max_tp : now + timeout;

        ordered_lock lock(m_val_mtx, m_exception_mtx);
        return m_cv.wait_until(lock, tp, [this]() { return m_val || m_exception; });
    }

    template<typename ThreadPool, typename T>
    auto future_controller<ThreadPool, T>::get_val()
    {
        if constexpr(!std::is_void_v<T>) {
            using val_t = decltype(m_val->to_underlying());
            using tuple_t = std::tuple<ordered_lock<decltype(m_val_mtx), decltype(m_outer_val_mtx)>, val_t>;

            ordered_lock lock(m_val_mtx, m_outer_val_mtx);
            assert(m_val);
            return tuple_t{ std::move(lock), m_val->to_underlying()};
        }
    }

    template<typename ThreadPool, typename T>
    auto future_controller<ThreadPool, T>::get_val() const
    {
        if constexpr(!std::is_void_v<T>) {
            using val_t = decltype(m_val->to_underlying());
            using tuple_t = std::tuple<ordered_lock<decltype(m_val_mtx), decltype(m_outer_val_mtx)>, val_t>;

            ordered_lock lock(m_val_mtx, m_outer_val_mtx);
            assert(m_val);
            return tuple_t{ std::move(lock), m_val->to_underlying() };
        }
    }

    template<typename ThreadPool, typename T>
    void future_controller<ThreadPool, T>::wait_data_ready_or_throw() const
    {
        wait();
        if(m_exception) {
            std::rethrow_exception(*m_exception);
        }
    }

    template<typename ThreadPool, typename T>
    void future_controller<ThreadPool, T>::process_on_success_async()
    {
        m_thread_pool->post([s = this->shared_from_this()] {
            std::queue<on_success_t> temp;

            {
                ordered_lock l(s->m_on_success_mtx, s->m_val_mtx, s->m_exception_mtx);
                assert(!(s->m_val && s->m_exception));

                if(s->m_val) {
                    temp.swap(s->m_on_success_queue);
                } else if (s->m_exception) {
                    temp.swap(s->m_on_success_queue);
                    return;
                } else {
                    return;
                }
            }

            while(!temp.empty()) {
                auto f = std::move(temp.front());
                temp.pop();
                s->m_thread_pool->post([s, f = std::move(f)]() mutable {
                    s->call_success(std::move(f));
                });
            }
        });
    }

    template<typename ThreadPool, typename T>
    void future_controller<ThreadPool, T>::process_on_fail_async()
    {
        m_thread_pool->post([s = this->shared_from_this()] {
            std::queue<on_fail_t> temp;

            {
                ordered_lock l(s->m_on_fail_mtx, s->m_val_mtx, s->m_exception_mtx);
                assert(!(s->m_val && s->m_exception));

                if(s->m_exception) {
                    temp.swap(s->m_on_fail_queue);
                } else if(s->m_val) {
                    temp.swap(s->m_on_fail_queue);
                    return;
                } else {
                    return;
                }
            }

            while(!temp.empty()) {
                auto f = std::move(temp.front());
                temp.pop();
                s->m_thread_pool->post([s, f = std::move(f)]() mutable {
                    s->call_fail(std::move(f));
                });
            }
        });
    }

    template<typename ThreadPool, typename T>
    void future_controller<ThreadPool, T>::call_success(on_success_t &&func)
    {
        assert(func);

        if constexpr(!std::is_void_v<T>) {
            ordered_lock guard(m_val_mtx, m_outer_val_mtx);
            assert(m_val);
            func(type_qualifiers_cast<std::add_rvalue_reference_t<T>>(m_val->to_underlying()));
        } else {
            func();
        }
    }

    template<typename ThreadPool, typename T>
    void future_controller<ThreadPool, T>::call_fail(on_fail_t &&func)
    {
        ordered_lock guard(m_exception_mtx);

        assert(m_exception);
        assert(func);
        func(*m_exception);
    }

    template<typename ThreadPool, typename T>
    void future_controller<ThreadPool, T>::add_child(std::unique_ptr<ifuture_controller> child)
    {
        m_children.lock()->push_back(std::move(child));
    }

    template<typename ThreadPool, typename T>
    void future_controller<ThreadPool, T>::add_dependent(std::shared_ptr<ifuture_controller> dependent)
    {
        m_dependent.lock()->push_back(std::move(dependent));
    }

    template<typename ThreadPool, typename T>
    std::mutex &future_controller<ThreadPool, T>::get_value_mtx_ref() const noexcept
    {
        if(m_outer_val_mtx.has_underlying()) {
            return m_outer_val_mtx.get_underlying();
        }
        return m_val_mtx.get_underlying();
    }
}
