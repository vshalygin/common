#pragma once
#include "future-value.h"
#include "future-value-state.h"

#include <common-lib/mpl/type-traits.h>
#include <common-lib/mpl/type-transform.h>
#include <common-lib/synchronization/ordered-mutex.h>
#include <common-lib/synchronization/ordered-lock.h>
#include <common-lib/synchronization/value-locker.h>
#include <common-lib/utils/value-proxy.h>
#include <common-lib/utils/function.h>
#include <common-lib/memory/enable-shared-from-this-manual-set.h>
#include <common-lib/utils/type-qualifiers-cast.h>

#include <memory>
#include <cassert>
#include <optional>
#include <mutex>
#include <condition_variable>
#include <functional>
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
        template<typename U>
        static auto create_on_success()
        {
            if constexpr(std::is_void_v<U>) {
                return function<void()>{};
            } else {
                return function<void(fvalue<ThreadPool, U>)>{};
            }
        }

        using on_success_t = decltype(create_on_success<T>());
        using on_fail_t = function<void(std::exception_ptr)>;

    public:
        explicit future_controller(ThreadPool *thread_pool);

        future_controller(const future_controller &) = delete;
        future_controller &operator=(const future_controller &) = delete;

        bool has_value() const;
        bool has_exception() const;

        template<typename Func>
        void set_on_success_and_fail(Func &&success_func, on_fail_t &&fail_func);

        template<typename TT = T, std::enable_if_t<std::is_void_v<TT>, int> = 0>
        void set_value();

        template<typename U, typename TT = T, std::enable_if_t<!std::is_void_v<TT>, int> = 0>
        void set_value(U &&val);

        template<typename U, typename SourceT, typename TT = T,
                 std::enable_if_t<std::is_reference_v<TT>, int> = 0>
        void set_value(
            U &&val,
            std::shared_ptr<future_value_state<SourceT>> source_value_state);

        void set_value_state(std::shared_ptr<future_value_state<T>> value_state);

        void set_exception(const std::exception_ptr &e);

        auto get() const;

        void wait() const;
        bool wait_for(std::chrono::milliseconds timeout) const;

        void add_child(std::unique_ptr<ifuture_controller> child);
        void add_dependent(std::shared_ptr<ifuture_controller> dependent);

    private:
        using value_proxy_t = cl::value_proxy<add_lvalue_ref_to_value_t<T>>;
        using on_success_mtx_t = ordered_mutex<0>;
        using on_fail_mtx_t = ordered_mutex<1>;
        using val_mtx_t = ordered_mutex<2>;
        using exception_mtx_t = ordered_mutex<3>;

        template<typename Func>
        void set_on_success_unsafe(Func &&func);

        void set_on_fail_unsafe(on_fail_t &&func);

        template<typename U>
        void set_value_impl(U &&val, std::shared_ptr<std::mutex> value_mutex);

        void wait_data_ready_or_throw() const;

        void process_on_success_async();
        void process_on_fail_async();

        void call_success(on_success_t &&func);
        void call_fail(on_fail_t &&func);

    private:
        ThreadPool *m_thread_pool;

        value_locker<std::vector<std::unique_ptr<ifuture_controller>>> m_children;
        value_locker<std::vector<std::shared_ptr<ifuture_controller>>> m_dependent;

        mutable on_success_mtx_t m_on_success_mtx;
        std::queue<on_success_t> m_on_success_queue;

        mutable on_fail_mtx_t m_on_fail_mtx;
        std::queue<on_fail_t> m_on_fail_queue;

        mutable val_mtx_t m_val_mtx;
        std::shared_ptr<future_value_state<T>> m_val_state;

        mutable exception_mtx_t m_exception_mtx;
        std::optional<std::exception_ptr> m_exception;

        mutable std::condition_variable_any m_cv;

    };

    template<typename ThreadPool, typename T>
    future_controller<ThreadPool, T>::future_controller(ThreadPool *thread_pool)
        : m_thread_pool(thread_pool)
    {
        assert(m_thread_pool);
    }

    template<typename ThreadPool, typename T>
    bool future_controller<ThreadPool, T>::has_value() const
    {
        ordered_lock guard(m_val_mtx);
        return m_val_state != nullptr;
    }

    template<typename ThreadPool, typename T>
    bool future_controller<ThreadPool, T>::has_exception() const
    {
        ordered_lock guard(m_exception_mtx);
        return m_exception.has_value();
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
        if constexpr(!std::is_void_v<T>) {
            static_assert(std::is_invocable_v<Func, fvalue<ThreadPool, T>>,
                          "success callback must accept fvalue");
            static_assert(std::is_same_v<
                              std::invoke_result_t<Func, fvalue<ThreadPool, T>>, void>,
                          "success callback must return void");
        } else {
            static_assert(std::is_invocable_v<Func>,
                          "success callback must have no arguments");
            static_assert(std::is_same_v<std::invoke_result_t<Func>, void>,
                          "success callback must return void");
        }

        if constexpr(std::is_void_v<T>) {
            m_on_success_queue.push(std::forward<Func>(func));
        } else {
            m_on_success_queue.push([func = std::forward<Func>(func)]
                                    (fvalue<ThreadPool, T> value) mutable {
                std::invoke(func, std::move(value));
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
        auto value_state = std::make_shared<future_value_state<T>>();
        value_state->set_value(value_proxy_t{});
        set_value_state(std::move(value_state));
    }

    template<typename ThreadPool, typename T>
    template<typename U, typename TT, std::enable_if_t<!std::is_void_v<TT>, int>>
    void future_controller<ThreadPool, T>::set_value(U &&val)
    {
        set_value_impl(std::forward<U>(val), {});
    }

    template<typename ThreadPool, typename T>
    template<typename U, typename SourceT, typename TT,
             std::enable_if_t<std::is_reference_v<TT>, int>>
    void future_controller<ThreadPool, T>::set_value(
        U &&val,
        std::shared_ptr<future_value_state<SourceT>> source_value_state)
    {
        assert(source_value_state);
        set_value_impl(
            std::forward<U>(val),
            source_value_state->m_mtx);
    }

    template<typename ThreadPool, typename T>
    template<typename U>
    void future_controller<ThreadPool, T>::set_value_impl(
        U &&val,
        std::shared_ptr<std::mutex> value_mutex)
    {
        value_proxy_t v;

        if constexpr(std::is_reference_v<T>) {
            v = value_proxy_t{ std::forward<U>(val), value_proxy_external };
        } else {
            v = value_proxy_t{ std::forward<U>(val), value_proxy_owned };
        }

        auto value_state = value_mutex
            ? std::make_shared<future_value_state<T>>(std::move(value_mutex))
            : std::make_shared<future_value_state<T>>();
        value_state->set_value(std::move(v));
        set_value_state(std::move(value_state));
    }

    template<typename ThreadPool, typename T>
    void future_controller<ThreadPool, T>::set_value_state(
        std::shared_ptr<future_value_state<T>> value_state)
    {
        assert(value_state);

        {
            ordered_lock g(m_val_mtx);
            assert(!m_val_state);
            m_val_state = std::move(value_state);

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
            ordered_lock lock(m_val_mtx);
            return fvalue<ThreadPool, T>(m_val_state);
        }
    }

    template<typename ThreadPool, typename T>
    void future_controller<ThreadPool, T>::wait() const
    {
        ordered_lock lock(m_val_mtx, m_exception_mtx);
        m_cv.wait(lock, [this]() { return m_val_state || m_exception; });
    }

    template<typename ThreadPool, typename T>
    bool future_controller<ThreadPool, T>::wait_for(std::chrono::milliseconds timeout) const
    {
        using clock = std::chrono::steady_clock;

        const auto now = clock::now();
        const auto max_tp = clock::time_point::max();

        clock::time_point tp = (timeout > max_tp - now) ? max_tp : now + timeout;

        ordered_lock lock(m_val_mtx, m_exception_mtx);
        return m_cv.wait_until(lock, tp, [this]() { return m_val_state || m_exception; });
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
                assert(!(s->m_val_state && s->m_exception));

                if(s->m_val_state) {
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
                assert(!(s->m_val_state && s->m_exception));

                if(s->m_exception) {
                    temp.swap(s->m_on_fail_queue);
                } else if(s->m_val_state) {
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
            std::shared_ptr<future_value_state<T>> value_state;
            {
                ordered_lock guard(m_val_mtx);
                value_state = m_val_state;
            }
            func(fvalue<ThreadPool, T>(std::move(value_state)));
        } else {
            func();
        }
    }

    template<typename ThreadPool, typename T>
    void future_controller<ThreadPool, T>::call_fail(on_fail_t &&func)
    {
        std::exception_ptr exception;
        {
            ordered_lock guard(m_exception_mtx);
            assert(m_exception);
            exception = *m_exception;
        }

        assert(func);
        func(std::move(exception));
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

}
