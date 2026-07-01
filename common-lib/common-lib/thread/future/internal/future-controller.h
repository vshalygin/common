#pragma once
#include "future-callback.h"
#include "future-callback-void.h"
#include "future-data.h"

#include <common-lib/mpl/type-traits.h>
#include <common-lib/synchronization/ordered-mutex.h>
#include <common-lib/synchronization/ordered-lock.h>
#include <common-lib/utils/type-wrapper.h>

#include <memory>
#include <cassert>
#include <functional>
#include <optional>
#include <mutex>
#include <condition_variable>

namespace vshalygin::cl::internal {
    template<typename T, typename ThreadPool>
    class future_controller
        : public std::enable_shared_from_this<future_controller<T, ThreadPool>>
    {
        class creator
        {};

    public:
        static std::shared_ptr<future_controller> create(ThreadPool *thread_pool);

        explicit future_controller(ThreadPool *thread_pool, creator);

        future_controller(const future_controller &) = delete;
        future_controller &operator=(const future_controller &) = delete;

        template<typename Func>
        void set_on_success(Func &&func);

        void set_on_fail(std::function<void(std::exception_ptr)> &&func);
        void set_on_fail_if_not_set(std::function<void(std::exception_ptr)> &&func);

        void set_value(auto&&...value);
        void set_exception(const std::exception_ptr &e);

        auto get() const;
        auto get();

        auto get_val();
        auto get_val() const;

    private:
        void wait_data_ready_or_throw() const;

        void post_success(bool need_notify);
        void post_fail(bool need_notify);

        void call_success(bool need_notify);
        void call_fail(bool need_notify);

    private:
        using type_wrapper = type_wrapper<T>;

        ThreadPool *m_thread_pool;

        mutable ordered_mutex<0> m_on_success_mtx;
        std::unique_ptr<ifuture_callback<T>> m_on_success;

        mutable ordered_mutex<1> m_on_fail_mtx;
        std::function<void(std::exception_ptr)> m_on_fail;

        mutable ordered_mutex<2> m_val_mtx;
        std::unique_ptr<type_wrapper> m_val;
        bool m_is_value_ready = false;

        mutable ordered_mutex<3> m_exception_mtx;
        std::optional<std::exception_ptr> m_exception;
        bool m_is_exception_ready = false;

        mutable std::condition_variable_any m_cv;
    };
}
