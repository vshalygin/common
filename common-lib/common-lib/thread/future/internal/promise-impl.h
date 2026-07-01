#pragma once
#include "promise.h"

namespace vshalygin::cl::internal {
    template<typename T, typename ThreadPool>
    promise<T, ThreadPool>::promise(ThreadPool *thread_pool)
        : m_thread_pool(thread_pool)
        , m_controller(std::make_shared<future_controller<T, ThreadPool>>(thread_pool))
        , m_future(thread_pool, m_controller)
    {
        assert(m_thread_pool);
    }

    template<typename T, typename ThreadPool>
    template<typename Function>
    promise<T, ThreadPool>::promise(ThreadPool *thread_pool,
                                    Function &&function)
        : m_thread_pool(thread_pool)
        , m_function(std::make_shared<promise_function<Function>>
                     (std::forward<Function>(function)))
        , m_controller(std::make_shared<future_controller<T, ThreadPool>>(thread_pool))
        , m_future(thread_pool, m_controller)
    {
        static_assert(function_arg_count_v<Function> == 0);
        assert(m_thread_pool);
    }

    template<typename T, typename ThreadPool>
    void promise<T, ThreadPool>::resolve()
    {
        if(!m_function) {
            throw std::logic_error("no resolve function");
        }

        m_thread_pool->post([controller = m_controller,
                            func = std::move(m_function)]() mutable {
            try {
                if constexpr(!std::is_void_v<T>) {
                    controller->set_value(func->call());
                } else {
                    func->call();
                    controller->set_value();
                }
            } catch(...) {
                controller->set_exception(std::current_exception());
            }
        });
    }

    template<typename T, typename ThreadPool>
    future<T, ThreadPool> promise<T, ThreadPool>::get_future()
    {
        if(!m_future.is_valid()) {
            throw std::logic_error("no future");
        }

        return std::move(m_future);
    }

    template<typename T, typename ThreadPool>
    bool promise<T, ThreadPool>::is_valid() const
    {
        return m_controller != nullptr;
    }

    template<typename T, typename ThreadPool>
    std::shared_ptr<future_controller<T, ThreadPool>>
        promise<T, ThreadPool>::get_controller() const
    {
        return m_controller;
    }
}
