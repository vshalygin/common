#pragma once
#include "future-controller.h"

namespace vshalygin::cl::internal {
    template<typename T, typename ThreadPool>
    class future_controller<T, ThreadPool>::notify_all_on_destruct
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
        static_assert(std::is_same_v<void, function_ret_t<Func>>,
                      "success callback return type is not void");
        static_assert(function_arg_count_v<Func> == 1,
                      "success callback arg count is not 1");
        static_assert(is_static_castable_v<T &&, function_arg_t<0, Func>>,
                      "value cannot be converted to success callback parameter");

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
        std::unique_lock guard(m_mtx);
        if(m_val || m_exception) {
            throw std::logic_error("value or exception already set");
        }

        m_val = std::make_unique<type_wrapper<T>>(std::forward<T>(value));

        if(m_on_success) {
            post_success(true);
        } else {
            guard.unlock();
            m_cv.notify_all();
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
        assert(m_val);
        m_on_success->call(std::move(m_val->to_underlying()));
    }

    template<typename T, typename ThreadPool>
    void future_controller<T, ThreadPool>::call_fail(bool need_notify)
    {
        notify_all_on_destruct n(need_notify ? &m_cv : nullptr);
        std::lock_guard guard(m_mtx);
        assert(m_exception);
        m_on_fail(*m_exception);
    }
}
