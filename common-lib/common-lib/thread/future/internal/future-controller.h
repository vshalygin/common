#pragma once
#include "future-callback.h"
#include "future-data.h"

#include <common-lib/mpl/type-traits.h>
#include <common-lib/utils/type-wrapper.h>

#include <memory>
#include <cassert>
#include <functional>
#include <optional>
#include <mutex>
#include <condition_variable>

namespace vshalygin::cl::internal {
    //TODO void specialization

    template<typename T, typename ThreadPool>
    class future_controller
        : public std::enable_shared_from_this<future_controller<T, ThreadPool>>
    {
        using this_type = future_controller<T, ThreadPool>;

        class notify_all_on_destruct;
        class set_true_on_destruct;
        class two_mutex_lock;

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

        future_data<const T, ThreadPool> get() const;
        future_data<T, ThreadPool> get();

        auto get_val()
        {
            using val_t = decltype(m_val->to_underlying());
            using tuple_t = std::tuple<std::unique_lock<std::mutex>, val_t>;

            assert(m_val);
            return tuple_t{ std::unique_lock{ m_val_mtx }, m_val->to_underlying() };
        }

        auto get_val() const
        {
            using val_t = decltype(m_val->to_underlying());
            using tuple_t = std::tuple<std::unique_lock<std::mutex>, val_t>;

            assert(m_val);
            return tuple_t{ std::unique_lock{ m_val_mtx }, m_val->to_underlying() };
        }

    private:
        void wait_data_ready_or_throw() const;

        void post_success(bool need_notify);
        void post_fail(bool need_notify);

        void call_success(bool need_notify);
        void call_fail(bool need_notify);

    private:
        ThreadPool *m_thread_pool;

        mutable std::mutex m_val_mtx;
        std::unique_ptr<type_wrapper<T>> m_val;
        bool m_is_value_ready = false;

        mutable std::mutex m_exception_mtx;
        std::optional<std::exception_ptr> m_exception;
        bool m_is_exception_ready = false;

        mutable std::mutex m_on_success_mtx;
        std::unique_ptr<ifuture_callback<T>> m_on_success;

        mutable std::mutex m_on_fail_mtx;
        std::function<void(std::exception_ptr)> m_on_fail;

        mutable std::condition_variable_any m_cv;
    };
}
