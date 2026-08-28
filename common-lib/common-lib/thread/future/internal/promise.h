#pragma once
#include "future-controller.h"
#include "future-store-type-or-self.h"
#include "is-future.h"

#include <common-lib/mpl/function-traits.h>
#include <common-lib/utils/function.h>

#include <memory>

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

        promise(promise &&) = default;
        promise &operator=(promise &&) = default;

        void resolve(Args...args);

        auto get_future();

        bool is_valid() const;

    private:
        ThreadPool *m_thread_pool = nullptr;

        function<R(Args...)> m_function;

        std::shared_ptr<future_controller<ThreadPool, future_store_type_or_self_t<R>>> m_controller;
        future<ThreadPool, future_store_type_or_self_t<R>> m_future;
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
    {
        assert(m_thread_pool);
        m_controller->set_self_shared_ptr(m_controller);
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
                        auto prev_controller_wp = std::weak_ptr(prev_controller);
                        auto on_success = [c = controller, prev_controller_wp]
                                          (future_store && /*value*/) mutable {
                            assert(!prev_controller_wp.expired());
                            c->set_value_state(
                                std::shared_ptr(prev_controller_wp)->get_value_state());
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
