#pragma once
#include "promise.h"

namespace vshalygin::cl::internal {
    template<typename ThreadPool, typename T, typename...ResolveArgs>
    promise<ThreadPool, T, ResolveArgs...>::promise(ThreadPool *thread_pool)
        : m_thread_pool(thread_pool)
        , m_controller(future_controller<T, ThreadPool>::create(thread_pool))
        , m_future(thread_pool, m_controller)
    {
        assert(m_thread_pool);
    }

    template<typename ThreadPool, typename T, typename...ResolveArgs>
    template<typename Function>
    promise<ThreadPool, T, ResolveArgs...>::promise(ThreadPool *thread_pool,
                                                    Function &&function)
        : m_thread_pool(thread_pool)
        , m_function(std::make_shared<promise_function<Function, T, ResolveArgs...>>
                                                        (std::forward<Function>(function)))
        , m_controller(future_controller<T, ThreadPool>::create(thread_pool))
        , m_future(thread_pool, m_controller)
    {
        assert(m_thread_pool);
    }

    template<typename ThreadPool, typename T, typename...ResolveArgs>
    void promise<ThreadPool, T, ResolveArgs...>::resolve(ResolveArgs...args)
    {
        if(!m_function) {
            throw std::logic_error("no resolve function");
        }

        m_thread_pool->post([controller = m_controller,
                             func = std::move(m_function),
                             args = std::tuple{ std::forward<ResolveArgs>(args)... }]() mutable {
            try {
                if constexpr(!std::is_void_v<T>) {
                    controller->set_value(std::apply([&func](auto&&...arg) -> decltype(auto) {
                        return func->call(std::move(arg)...);
                    }, std::move(args)));
                } else {
                    std::apply([&func](auto&&...arg) {
                        func->call(std::move(arg)...);
                    }, std::move(args));
                    controller->set_value();
                }
            } catch(...) {
                controller->set_exception(std::current_exception());
            }
        });
    }

    template<typename ThreadPool, typename T, typename...ResolveArgs>
    auto promise<ThreadPool, T, ResolveArgs...>::get_future()
    {
        if(!m_future.is_valid()) {
            throw std::logic_error("no future");
        }

        return std::move(m_future);
    }

    template<typename ThreadPool, typename T, typename...ResolveArgs>
    bool promise<ThreadPool, T, ResolveArgs...>::is_valid() const
    {
        return m_controller != nullptr;
    }

    template<typename ThreadPool, typename T, typename...ResolveArgs>
    auto promise<ThreadPool, T, ResolveArgs...>::get_controller() const
    {
        return m_controller;
    }
}
