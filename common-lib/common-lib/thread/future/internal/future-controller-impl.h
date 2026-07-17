#pragma once
#include "future-controller.h"
#include <common-lib/utils/type-qualifiers-cast.h>
#include <common-lib/mpl/tuple-traits.h>

namespace vshalygin::cl::internal {
    template<typename ThreadPool, typename T>
    future_controller<ThreadPool, T>::future_controller(ThreadPool *thread_pool)
        : m_thread_pool(thread_pool)
    {
        assert(m_thread_pool);
    }

    template<typename ThreadPool, typename T>
    template<typename Func>
    void future_controller<ThreadPool, T>::set_on_success(Func &&func)
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

        ordered_lock guard(m_on_success_mtx);
        if constexpr(std::is_void_v<T>) {
            m_on_success_queue.push(std::forward<Func>(func));
        } else {
            m_on_success_queue.push([func = std::forward<Func>(func)](std::add_rvalue_reference_t<T> val) mutable {
                func(type_qualifiers_cast<function_arg_t<0, Func>>(val));
            });
        }

        ordered_lock guard2(push_back(std::move(guard), m_val_mtx));
        if(m_val) {
            post_success();
        }
    }

    template<typename ThreadPool, typename T>
    void future_controller<ThreadPool, T>::set_on_fail(
        function<void(std::exception_ptr)> &&func)
    {
        ordered_lock guard(m_on_fail_mtx);

        m_on_fail_queue.push(std::move(func));

        ordered_lock g(push_back(std::move(guard), m_exception_mtx));
        if(m_exception) {
            post_fail();
        }
    }

    template<typename ThreadPool, typename T>
    void future_controller<ThreadPool, T>::set_value(auto&&...value)
    {
        if constexpr(std::is_void_v<T>) {
            static_assert(sizeof...(value) == 0);
        } else {
            static_assert(sizeof...(value) == 1);
        }

        {
            ordered_lock g(m_on_success_mtx, m_val_mtx, m_exception_mtx);
            assert(!m_val && !m_exception);

            if constexpr(std::is_void_v<T>) {
                m_val.emplace(value_proxy{});
            } else if constexpr(std::is_reference_v<T>) {
                m_val.emplace(value_proxy{ std::forward<decltype(value)>(value)...,
                                           value_proxy_external});
            } else {
                m_val.emplace(value_proxy{ std::forward<decltype(value)>(value)...,
                                           value_proxy_owned });
            }

            post_success();
        }

        m_cv.notify_all();
    }

    template<typename ThreadPool, typename T>
    void future_controller<ThreadPool, T>::set_exception(const std::exception_ptr &e)
    {
        {
            ordered_lock g(m_on_fail_mtx, m_val_mtx, m_exception_mtx);
            assert(!m_val && !m_exception);

            m_exception = e;

            post_fail();
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
    auto future_controller<ThreadPool, T>::get_val()
    {
        if constexpr(!std::is_void_v<T>) {
            using val_t = decltype(m_val->to_underlying());
            using tuple_t = std::tuple<ordered_lock<decltype(m_val_mtx)>, val_t>;

            ordered_lock lock(m_val_mtx);
            assert(m_val);
            return tuple_t{ std::move(lock), m_val->to_underlying()};
        }
    }

    template<typename ThreadPool, typename T>
    auto future_controller<ThreadPool, T>::get_val() const
    {
        if constexpr(!std::is_void_v<T>) {
            using val_t = decltype(m_val->to_underlying());
            using tuple_t = std::tuple<ordered_lock<decltype(m_val_mtx)>, val_t>;

            ordered_lock lock(m_val_mtx);
            assert(m_val);
            return tuple_t{ std::move(lock), m_val->to_underlying() };
        }
    }

    template<typename ThreadPool, typename T>
    void future_controller<ThreadPool, T>::wait_data_ready_or_throw() const
    {
        ordered_lock lock(m_val_mtx, m_exception_mtx);
        m_cv.wait(lock, [this]() { return m_val || m_exception; });
        if(m_exception) {
            std::rethrow_exception(*m_exception);
        }
    }

    template<typename ThreadPool, typename T>
    void future_controller<ThreadPool, T>::post_success()
    {
        while(!m_on_success_queue.empty()) {
            auto f = std::move(m_on_success_queue.front());
            m_on_success_queue.pop();
            m_thread_pool->post([s = this->shared_from_this(), f = std::move(f)]() mutable {
                s->call_success(std::move(f));
            });
        }
    }

    template<typename ThreadPool, typename T>
    void future_controller<ThreadPool, T>::post_fail()
    {
        while(!m_on_fail_queue.empty()) {
            auto f = std::move(m_on_fail_queue.front());
            m_on_fail_queue.pop();
            m_thread_pool->post([s = this->shared_from_this(), f = std::move(f)]() mutable {
                s->call_fail(std::move(f));
            });
        }
    }

    template<typename ThreadPool, typename T>
    void future_controller<ThreadPool, T>::call_success(on_success_t &&func)
    {
        ordered_lock guard(m_val_mtx);

        assert(m_val);
        assert(func);
        if constexpr(!std::is_void_v<T>) {
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
}
