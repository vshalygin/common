#pragma once
#include "future-controller.h"

namespace vshalygin::cl::internal {
    template<typename ThreadPool>
    class future_controller<void, ThreadPool>
        : public std::enable_shared_from_this<future_controller<void, ThreadPool>>
    {
        using this_type = future_controller<void, ThreadPool>;

        class notify_all_on_destruct;
        class set_true_on_destruct;
        class two_mutex_lock;
        //TODO add shared ptr creator
    public:
        explicit future_controller(ThreadPool *thread_pool);

        future_controller(const future_controller &) = delete;
        future_controller &operator=(const future_controller &) = delete;

        template<typename Func>
        void set_on_success(Func &&func);

        void set_on_fail(std::function<void(std::exception_ptr)> &&func);
        void set_on_fail_if_not_set(std::function<void(std::exception_ptr)> &&func);

        void set_value();
        void set_exception(const std::exception_ptr &e);

        void get() const;

    private:
        void wait_value_ready_or_throw() const;

        void post_success(bool need_notify);
        void post_fail(bool need_notify);

        void call_success(bool need_notify);
        void call_fail(bool need_notify);

    private:
        ThreadPool *m_thread_pool;

        mutable std::mutex m_val_mtx;
        bool m_is_value_set = false;
        bool m_is_value_ready = false;

        mutable std::mutex m_exception_mtx;
        std::optional<std::exception_ptr> m_exception;
        bool m_is_exception_ready = false;

        mutable std::mutex m_on_success_mtx;
        std::unique_ptr<ifuture_callback<void>> m_on_success;

        mutable std::mutex m_on_fail_mtx;
        std::function<void(std::exception_ptr)> m_on_fail;

        mutable std::condition_variable_any m_cv;
    };
}
