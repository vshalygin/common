#pragma once
#include "future-data.h"

#include <common-lib/mpl/type-traits.h>
#include <common-lib/mpl/type-transform.h>
#include <common-lib/synchronization/ordered-mutex.h>
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
        void set_on_success(Func &&func);

        void set_on_fail(function<void(std::exception_ptr)> &&func);

        void set_value(auto&&...value);
        void set_reference(auto&&...value);

        void set_exception(const std::exception_ptr &e);

        auto get() const;
        auto get();

        auto get_val();
        auto get_val() const;

        void add_child(std::unique_ptr<ifuture_controller> child);

    private:
        using value_proxy = value_proxy<add_lvalue_ref_to_value_t<T>>;

        void set_success(value_proxy value);

        void wait_data_ready_or_throw() const;

        void post_success();
        void post_fail();

        void call_success(on_success_t &&func);
        void call_fail(on_fail_t &&func);

    private:
        ThreadPool *m_thread_pool;

        value_locker<std::vector<std::unique_ptr<ifuture_controller>>> m_children;

        mutable ordered_mutex<0> m_on_success_mtx;
        std::queue<on_success_t> m_on_success_queue;

        mutable ordered_mutex<1> m_on_fail_mtx;
        std::queue<on_fail_t> m_on_fail_queue;

        mutable ordered_mutex<2> m_val_mtx;
        std::optional<value_proxy> m_val;

        mutable ordered_mutex<3> m_exception_mtx;
        std::optional<std::exception_ptr> m_exception;

        mutable std::condition_variable_any m_cv;
    };
}
