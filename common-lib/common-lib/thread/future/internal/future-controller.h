#pragma once
#include "future-data.h"

#include <common-lib/mpl/type-traits.h>
#include <common-lib/synchronization/ordered-mutex.h>
#include <common-lib/synchronization/ordered-lock.h>
#include <common-lib/utils/type-wrapper.h>
#include <common-lib/utils/function.h>

#include <memory>
#include <cassert>
#include <optional>
#include <mutex>
#include <condition_variable>
#include <queue>

namespace vshalygin::cl::internal {
    template<typename ThreadPool, typename T>
    class future_controller
        : public std::enable_shared_from_this<future_controller<ThreadPool, T>>
    {
        using on_success_t = function<void(std::add_rvalue_reference_t<T>)>;
        using on_fail_t = function<void(std::exception_ptr)>;

        class creator
        {};

    public:
        static std::shared_ptr<future_controller> create(ThreadPool *thread_pool);

        explicit future_controller(ThreadPool *thread_pool, creator);

        future_controller(const future_controller &) = delete;
        future_controller &operator=(const future_controller &) = delete;

        template<typename Func>
        void set_on_success(Func &&func);

        void set_on_fail(function<void(std::exception_ptr)> &&func);

        void set_value(auto&&...value);
        void set_exception(const std::exception_ptr &e);

        auto get() const;
        auto get();

        auto get_val();
        auto get_val() const;

    private:
        void wait_data_ready_or_throw() const;

        void post_success();
        void post_fail();

        void call_success(on_success_t &&func);
        void call_fail(on_fail_t &&func);

    private:
        using type_wrapper = type_wrapper<T>;

        ThreadPool *m_thread_pool;

        mutable ordered_mutex<0> m_on_success_mtx;
        std::queue<on_success_t> m_on_success_queue;

        mutable ordered_mutex<1> m_on_fail_mtx;
        std::queue<on_fail_t> m_on_fail_queue;

        mutable ordered_mutex<2> m_val_mtx;
        std::optional<type_wrapper> m_val;

        mutable ordered_mutex<3> m_exception_mtx;
        std::optional<std::exception_ptr> m_exception;

        mutable std::condition_variable_any m_cv;
    };
}
