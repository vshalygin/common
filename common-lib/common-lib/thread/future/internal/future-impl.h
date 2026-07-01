#pragma once
#include "future.h"

namespace vshalygin::cl::internal {
    template<typename T, typename ThreadPool>
    future<T, ThreadPool>::future(
                         ThreadPool *thread_pool,
                         std::shared_ptr<future_controller<T, ThreadPool>> controller)
        : m_thread_pool(thread_pool)
        , m_controller(std::move(controller))
    {
        assert(m_thread_pool);
    }

    template<typename T, typename ThreadPool>
    auto future<T, ThreadPool>::get() const
    {
        if(!m_controller) {
            throw std::logic_error("future is invalid");
        }

        return m_controller->get();
    }

    template<typename T, typename ThreadPool>
    template<typename Func>
    future<function_ret_t<Func>, ThreadPool> future<T, ThreadPool>::then(Func &&task)
    {
        using ret_t = function_ret_t<Func>;

        promise<ret_t, ThreadPool> promise(m_thread_pool);

        if constexpr(!std::is_void_v<T>) {
            m_controller->set_on_success([controller = promise.get_controller(),
                                          task = std::forward<Func>(task)](T &&val) mutable {
                try {
                    if constexpr(!std::is_void_v<ret_t>) {
                        controller->set_value(task(std::forward<T>(val)));
                    } else {
                        task(std::forward<T &&>(val));
                        controller->set_value();
                    }
                } catch(...) {
                    controller->set_exception(std::current_exception());
                }
            });
        } else {
            m_controller->set_on_success([controller = promise.get_controller(),
                                          task = std::forward<Func>(task)]() mutable {
                try {
                    if constexpr(!std::is_void_v<ret_t>) {
                        controller->set_value(task());
                    } else {
                        task();
                        controller->set_value();
                    }
                } catch(...) {
                    controller->set_exception(std::current_exception());
                }
            });
        }
        auto fail = [controller = promise.get_controller()](std::exception_ptr e) {
            controller->set_exception(e);
        };
        m_controller->set_on_fail_if_not_set(std::move(fail));

        return promise.get_future();
    }

    template<typename T, typename ThreadPool>
    void future<T, ThreadPool>::catched(
            std::function<void(std::exception_ptr)> &&task)
    {
        m_controller->set_on_fail(std::move(task));
    }

    template<typename T, typename ThreadPool>
    future<T, ThreadPool> future<T, ThreadPool>::catch_and_release_itself(
                                     std::function<void(std::exception_ptr)> &&task)
    {
        m_controller->set_on_fail(std::move(task));
        return std::move(*this);
    }

    template<typename T, typename ThreadPool>
    bool future<T, ThreadPool>::is_valid() const
    {
        return m_controller != nullptr;
    }
}
