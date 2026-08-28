#pragma once
#include "future-controller.h"
#include "future-store-type-or-self.h"
#include "is-future.h"

#include <common-lib/mpl/function-traits.h>
#include <common-lib/utils/function.h>

#include <future>
#include <memory>
#include <utility>

namespace vshalygin::cl::internal {
    template<typename ThreadPool, typename Signature>
    class promise;

    template<typename ThreadPool, typename R, typename...Args>
    class promise<ThreadPool, R(Args...)>
    {
        template<typename, typename>
        friend class promise;

    public:
        promise() = default;

        template<typename Function>
        explicit promise(ThreadPool *thread_pool,
                         Function &&function);

        promise(const promise &) = delete;
        promise &operator=(const promise &) = delete;

        promise(promise &&other) noexcept;
        promise &operator=(promise &&other) noexcept;

        ~promise() noexcept;

        void resolve(Args...args);

        auto get_future();

        bool is_valid() const;

    private:
        void abandon() noexcept;

    private:
        ThreadPool *m_thread_pool = nullptr;

        function<R(Args...)> m_function;

        std::shared_ptr<future_controller<ThreadPool, future_store_type_or_self_t<R>>> m_controller;
        future<ThreadPool, future_store_type_or_self_t<R>> m_future;
        bool m_completion_pending = false;
    };

    template<typename ThreadPool, typename Function>
    promise(ThreadPool *thread_pool, Function &&function)
        -> promise<ThreadPool, function_signature_t<Function>>;

    template<typename ThreadPool, typename R, typename...Args>
    template<typename Function>
    promise<ThreadPool, R(Args...)>::promise(ThreadPool *thread_pool,
                                             Function &&function)
        : m_thread_pool(thread_pool)
        , m_function(std::forward<Function>(function))
        , m_controller(std::make_shared<future_controller<ThreadPool, future_store_type_or_self_t<R>>>(thread_pool))
        , m_future(thread_pool, m_controller)
        , m_completion_pending(true)
    {
        assert(m_thread_pool);
        m_controller->set_self_shared_ptr(m_controller);
    }

    template<typename ThreadPool, typename R, typename...Args>
    promise<ThreadPool, R(Args...)>::promise(promise &&other) noexcept
        : m_thread_pool(std::exchange(other.m_thread_pool, nullptr))
        , m_function(std::move(other.m_function))
        , m_controller(std::move(other.m_controller))
        , m_future(std::move(other.m_future))
        , m_completion_pending(std::exchange(other.m_completion_pending, false))
    {}

    template<typename ThreadPool, typename R, typename...Args>
    promise<ThreadPool, R(Args...)> &
        promise<ThreadPool, R(Args...)>::operator=(promise &&other) noexcept
    {
        if(this != &other) {
            abandon();

            m_thread_pool = std::exchange(other.m_thread_pool, nullptr);
            m_function = std::move(other.m_function);
            m_controller = std::move(other.m_controller);
            m_future = std::move(other.m_future);
            m_completion_pending = std::exchange(other.m_completion_pending, false);
        }

        return *this;
    }

    template<typename ThreadPool, typename R, typename...Args>
    promise<ThreadPool, R(Args...)>::~promise() noexcept
    {
        abandon();
    }

    template<typename ThreadPool, typename R, typename...Args>
    void promise<ThreadPool, R(Args...)>::abandon() noexcept
    {
        if(!m_completion_pending || !m_controller) {
            return;
        }

        m_completion_pending = false;

        try {
            const auto exception = std::make_exception_ptr(
                std::future_error(std::future_errc::broken_promise));
            m_controller->set_exception(exception);
        } catch(...) {
        }
    }

    template<typename ThreadPool, typename R, typename...Args>
    void promise<ThreadPool, R(Args...)>::resolve(Args...args)
    {
        if(!m_function) {
            throw std::logic_error("no resolve function");
        }

        if constexpr(is_future_v<R>) {
            static_assert(is_value_v<R>);

            using future_t = R;
            using future_store = future_store_type_or_self_t<future_t>;

            m_thread_pool->post([controller = m_controller,
                                func = std::move(m_function),
                                args = std::tuple{ std::forward<Args>(args)... }]() mutable {
                try {
                    auto future = std::apply([&func](auto&&...arg) -> decltype(auto) {
                        return func(std::move(arg)...);
                    }, std::move(args));

                    auto on_fail = [c = controller](std::exception_ptr e) mutable {
                        c->set_exception(e);
                    };

                    auto prev_controller = future.get_controller();
                    if constexpr(std::is_void_v<future_store>) {
                        auto on_success = [c = controller]() mutable {
                            c->set_value();
                        };
                        prev_controller->set_on_success_and_fail(std::move(on_success), std::move(on_fail));
                    } else {
                        auto on_success = [c = controller] (fvalue<ThreadPool, future_store> value) mutable {
                            c->set_value_state(value.get_value_state());
                        };
                        prev_controller->set_on_success_and_fail(std::move(on_success), std::move(on_fail));
                    }
                } catch(...) {
                    controller->set_exception(std::current_exception());
                }
            });
        } else {
            m_thread_pool->post([controller = m_controller,
                                func = std::move(m_function),
                                args = std::tuple{ std::forward<Args>(args)... }]() mutable {
                try {
                    if constexpr(!std::is_void_v<R>) {
                        controller->set_value(std::apply([&func](auto&&...arg) -> decltype(auto) {
                            return func(std::move(arg)...);
                        }, std::move(args)));
                    } else {
                        std::apply([&func](auto&&...arg) {
                            func(std::move(arg)...);
                        }, std::move(args));
                        controller->set_value();
                    }
                } catch(...) {
                    controller->set_exception(std::current_exception());
                }
            });
        }

        m_completion_pending = false;
    }

    template<typename ThreadPool, typename R, typename...Args>
    auto promise<ThreadPool, R(Args...)>::get_future()
    {
        if(!m_future.is_valid()) {
            throw std::logic_error("no future");
        }

        return std::move(m_future);
    }

    template<typename ThreadPool, typename R, typename...Args>
    bool promise<ThreadPool, R(Args...)>::is_valid() const
    {
        return m_controller != nullptr;
    }
}
