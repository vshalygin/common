#pragma once
#include "future-callback.h"
#include "future-data.h"

#include <common-lib/mpl/type-traits.h>
#include <common-lib/mpl/type-wrapper.h>

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

        friend class future_data<T, ThreadPool>;

        class notify_all_on_destruct;

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

    private:
        void wait_data_ready_or_throw() const;

        void post_success(bool need_notify);
        void post_fail(bool need_notify);

        void call_success(bool need_notify);
        void call_fail(bool need_notify);

        decltype(auto) get_val()
        {
            assert(m_val);
            if constexpr(!std::is_reference_v<T>) {
                return (*m_val);
            } else {
                return *m_val;
            }
        }

        decltype(auto) get_val() const
        {
            assert(m_val);
            if constexpr(!std::is_reference_v<T>) {
                return (*m_val);
            } else {
                return *m_val;
            }
        }

    private:
        ThreadPool *m_thread_pool;

        std::unique_ptr<type_wrapper<T>> m_val;
        std::optional<std::exception_ptr> m_exception;

        std::unique_ptr<ifuture_callback<remove_c_ref_t<T>>> m_on_success;
        std::function<void(std::exception_ptr)> m_on_fail;

        mutable std::mutex m_mtx;
        mutable std::condition_variable m_cv;
    };
}
