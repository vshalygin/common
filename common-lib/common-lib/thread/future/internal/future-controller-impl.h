#pragma once
#include "future-controller.h"

namespace vshalygin::cl::internal {
    template<typename T, typename ThreadPool>
    class future_controller<T, ThreadPool>::notify_all_on_destruct
    {
    public:
        explicit notify_all_on_destruct(std::condition_variable_any *cv)
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
        std::condition_variable_any *m_cv = nullptr;
    };

    template<typename T, typename ThreadPool>
    class future_controller<T, ThreadPool>::set_true_on_destruct
    {
    public:
        explicit set_true_on_destruct(bool &val)
            : m_val(val)
        {}

        set_true_on_destruct(const set_true_on_destruct &) = delete;
        set_true_on_destruct &operator=(const set_true_on_destruct &) = delete;

        ~set_true_on_destruct()
        {
            m_val = true;
        }

    private:
        bool &m_val;
    };

    template<typename T, typename ThreadPool>
    class future_controller<T, ThreadPool>::two_mutex_lock
    {
    public:
        two_mutex_lock(std::mutex &mtx1, std::mutex &mtx2)
            : m_lock1(mtx1)
            , m_lock2(mtx2)
        {}

        two_mutex_lock(const two_mutex_lock &) = delete;
        two_mutex_lock &operator=(const two_mutex_lock &) = delete;

        void lock()
        {
            m_lock1.lock();
            m_lock2.lock();
        }

        void unlock()
        {
            m_lock2.unlock();
            m_lock1.unlock();
        }

    private:
        std::unique_lock<std::mutex> m_lock1;
        std::unique_lock<std::mutex> m_lock2;
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

        std::lock_guard guard(m_on_success_mtx);
        if(m_on_success) {
            throw std::logic_error("success handler already set");
        }

        m_on_success = std::make_unique<future_callback<T, Func>>
                                              (std::forward<Func>(func));

        std::lock_guard g(m_val_mtx);
        if(m_val) {
            post_success(false);
        }
    }

    template<typename T, typename ThreadPool>
    void future_controller<T, ThreadPool>::set_on_fail(
        std::function<void(std::exception_ptr)> &&func)
    {
        std::lock_guard guard(m_on_fail_mtx);
        if(m_on_fail) {
            throw std::logic_error("fail handler already set");
        }

        m_on_fail = std::move(func);

        std::lock_guard g(m_exception_mtx);
        if(m_exception) {
            post_fail(false);
        }
    }

    template<typename T, typename ThreadPool>
    void future_controller<T, ThreadPool>::set_on_fail_if_not_set(
        std::function<void(std::exception_ptr)> &&func)
    {
        std::lock_guard guard(m_on_fail_mtx);
        if(!m_on_fail) {
            m_on_fail = std::move(func);

            std::lock_guard g(m_exception_mtx);
            if(m_exception) {
                post_fail(false);
            }
        }
    }

    template<typename T, typename ThreadPool>
    void future_controller<T, ThreadPool>::set_value(T &&value)
    {
        bool need_notify = false;

        {
            std::lock_guard g1(m_on_success_mtx);
            std::lock_guard g2(m_val_mtx);
            std::lock_guard g3(m_exception_mtx);
            if(m_val || m_exception) {
                throw std::logic_error("value or exception already set");
            }

            m_val = std::make_unique<type_wrapper<T>>(std::forward<T>(value));

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
            std::lock_guard g1(m_on_fail_mtx);
            std::lock_guard g2(m_val_mtx);
            std::lock_guard g3(m_exception_mtx);
            if(m_val || m_exception) {
                throw std::logic_error("value or exeption already set");
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
    future_data<const T, ThreadPool>
        future_controller<T, ThreadPool>::get() const
    {
        wait_data_ready_or_throw();
        return future_data<T, ThreadPool>(this->shared_from_this());
    }

    template<typename T, typename ThreadPool>
    future_data<T, ThreadPool> future_controller<T, ThreadPool>::get()
    {
        wait_data_ready_or_throw();
        return future_data<T, ThreadPool>(this->shared_from_this());
    }

    template<typename T, typename ThreadPool>
    void future_controller<T, ThreadPool>::wait_data_ready_or_throw() const
    {
        two_mutex_lock lock(m_val_mtx, m_exception_mtx);
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
        notify_all_on_destruct n(need_notify ? &m_cv : nullptr);
        std::lock_guard guard(m_val_mtx);
        set_true_on_destruct s(m_is_value_ready);

        //m_on_success here is defined and will not change,
        //no need m_on_success_mtx block
        assert(m_val);
        m_on_success->call(std::move(m_val->to_underlying()));
    }

    template<typename T, typename ThreadPool>
    void future_controller<T, ThreadPool>::call_fail(bool need_notify)
    {
        notify_all_on_destruct n(need_notify ? &m_cv : nullptr);
        std::lock_guard guard(m_exception_mtx);
        set_true_on_destruct s(m_is_exception_ready);

        //m_exception here is defined and will not change,
        //no need m_on_fail_mtx block
        assert(m_exception);
        m_on_fail(*m_exception);
    }
}
