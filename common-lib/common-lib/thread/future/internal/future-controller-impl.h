#pragma once
#include "future-controller.h"
#include <common-lib/utils/do-on-destruct.h>
#include <common-lib/mpl/tuple-traits.h>

namespace vshalygin::cl::internal {
    template<typename T, typename ThreadPool>
    std::shared_ptr<future_controller<T, ThreadPool>>
        future_controller<T, ThreadPool>::create(ThreadPool *thread_pool)
    {
        return std::make_shared<future_controller>(thread_pool, creator{});
    }

    template<typename T, typename ThreadPool>
    future_controller<T, ThreadPool>::future_controller(ThreadPool *thread_pool, creator)
        : m_thread_pool(thread_pool)
    {
        assert(m_thread_pool);
    }

    template<typename T, typename ThreadPool>
    template<typename Func>
    void future_controller<T, ThreadPool>::set_on_success(Func &&func)
    {
        static_assert(std::is_same_v<void, function_ret_t<Func>>,
                      "success callback return type is not void");
        if constexpr(!std::is_void_v<T>) {
            static_assert(function_arg_count_v<Func> == 1,
                          "success callback arg count is not 1");
            static_assert(is_lvalue_static_castable_v<T &&, function_arg_t<0, Func>>,
                          "value cannot be converted to success callback parameter");
        } else {
            static_assert(function_arg_count_v<Func> == 0,
                          "success callback arg count is not 0");
        }

        ordered_lock guard(m_on_success_mtx);
        if(m_on_success) {
            throw std::logic_error("success handler already set");
        }

        m_on_success = std::make_unique<future_callback<T, Func>>
                                              (std::forward<Func>(func));

        ordered_lock guard2(push_back(std::move(guard), m_val_mtx));
        if(m_val) {
            post_success(false);
        }
    }

    template<typename T, typename ThreadPool>
    void future_controller<T, ThreadPool>::set_on_fail(
        std::function<void(std::exception_ptr)> &&func)
    {
        ordered_lock guard(m_on_fail_mtx);
        if(m_on_fail) {
            throw std::logic_error("fail handler already set");
        }

        m_on_fail = std::move(func);

        ordered_lock g(push_back(std::move(guard), m_exception_mtx));
        if(m_exception) {
            post_fail(false);
        }
    }

    template<typename T, typename ThreadPool>
    void future_controller<T, ThreadPool>::set_on_fail_if_not_set(
        std::function<void(std::exception_ptr)> &&func)
    {
        ordered_lock guard(m_on_fail_mtx);
        if(!m_on_fail) {
            m_on_fail = std::move(func);

            ordered_lock g(push_back(std::move(guard), m_exception_mtx));
            if(m_exception) {
                post_fail(false);
            }
        }
    }

    template<typename T, typename ThreadPool>
    void future_controller<T, ThreadPool>::set_value(auto&&...value)
    {
        if constexpr(std::is_void_v<T>) {
            static_assert(sizeof...(value) == 0);
        } else {
            static_assert(sizeof...(value) == 1);
        }

        bool need_notify = false;

        {
            ordered_lock g(m_on_success_mtx, m_val_mtx, m_exception_mtx);
            if(m_val || m_exception) {
                throw std::logic_error("value or exception already set");
            }

            if constexpr(std::is_void_v<T>) {
                m_val = std::make_unique<type_wrapper>();
            } else {
                m_val = std::make_unique<type_wrapper>(std::forward<decltype(value)>(value)...);
            }

            if(m_on_success) {
                post_success(true);
            } else {
                m_is_value_ready = true;
                need_notify = true;
            }
        }

        if(need_notify) {
            m_cv.notify_all();
        }
    }

    template<typename T, typename ThreadPool>
    void future_controller<T, ThreadPool>::set_exception(const std::exception_ptr &e)
    {
        bool need_notify = false;

        {
            ordered_lock g(m_on_fail_mtx, m_val_mtx, m_exception_mtx);
            if(m_val || m_exception) {
                throw std::logic_error("value or exception already set");
            }

            m_exception = e;
            if(m_on_fail) {
                post_fail(true);
            } else {
                m_is_exception_ready = true;
                need_notify = true;
            }
        }

        if(need_notify) {
            m_cv.notify_all();
        }
    }

    template<typename T, typename ThreadPool>
    auto future_controller<T, ThreadPool>::get() const
    {
        wait_data_ready_or_throw();
        if constexpr(!std::is_void_v<T>) {
            return future_data<T, ThreadPool>(this->shared_from_this());
        }
    }

    template<typename T, typename ThreadPool>
    auto future_controller<T, ThreadPool>::get()
    {
        wait_data_ready_or_throw();
        if constexpr(!std::is_void_v<T>) {
            return future_data<T, ThreadPool>(this->shared_from_this());
        }
    }

    template<typename T, typename ThreadPool>
    auto future_controller<T, ThreadPool>::get_val()
    {
        if constexpr(!std::is_void_v<T>) {
            using val_t = decltype(m_val->to_underlying());
            using tuple_t = std::tuple<ordered_lock<decltype(m_val_mtx)>, val_t>;

            ordered_lock lock(m_val_mtx);
            assert(m_val);
            return tuple_t{ std::move(lock), m_val->to_underlying()};
        }
    }

    template<typename T, typename ThreadPool>
    auto future_controller<T, ThreadPool>::get_val() const
    {
        if constexpr(!std::is_void_v<T>) {
            using val_t = decltype(m_val->to_underlying());
            using tuple_t = std::tuple<ordered_lock<decltype(m_val_mtx)>, val_t>;

            ordered_lock lock(m_val_mtx);
            assert(m_val);
            return tuple_t{ std::move(lock), m_val->to_underlying() };
        }
    }

    template<typename T, typename ThreadPool>
    void future_controller<T, ThreadPool>::wait_data_ready_or_throw() const
    {
        ordered_lock lock(m_val_mtx, m_exception_mtx);
        m_cv.wait(lock, [this]() { return m_is_value_ready || m_is_exception_ready; });
        if(m_exception) {
            std::rethrow_exception(*m_exception);
        }
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
        do_on_destruct d1([this, need_notify]() { if(need_notify) m_cv.notify_all(); });
        ordered_lock guard(m_val_mtx);
        do_on_destruct d2([this, need_notify]() { m_is_value_ready = true; });

        //m_on_success here is defined and will not change,
        //no need m_on_success_mtx block
        assert(m_val);
        assert(m_on_success);
        if constexpr(!std::is_void_v<T>) {
            m_on_success->call(static_cast<typename type_wrapper::type &&>(m_val->to_underlying()));
        } else {
            m_on_success->call();
        }
    }

    template<typename T, typename ThreadPool>
    void future_controller<T, ThreadPool>::call_fail(bool need_notify)
    {
        do_on_destruct d1([this, need_notify]() { if(need_notify) m_cv.notify_all(); });
        ordered_lock guard(m_exception_mtx);
        do_on_destruct d2([this, need_notify]() { m_is_exception_ready = true; });

        //m_on_fail here is defined and will not change,
        //no need m_on_fail_mtx block
        assert(m_exception);
        assert(m_on_fail);
        m_on_fail(*m_exception);
    }
}
