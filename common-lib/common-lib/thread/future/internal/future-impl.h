#pragma once
#include "is-future.h"
#include "future-store-type.h"

#include <common-lib/mpl/type-transform.h>
#include <common-lib/mpl/type-traits.h>

#include "future.h"

namespace vshalygin::cl::internal {
    template<typename ThreadPool, typename T>
    future<ThreadPool, T>::future(
                         ThreadPool *thread_pool,
                         std::shared_ptr<future_controller<T, ThreadPool>> controller)
        : m_thread_pool(thread_pool)
        , m_controller(std::move(controller))
    {
        assert(m_thread_pool);
    }

    template<typename ThreadPool, typename T>
    auto future<ThreadPool, T>::get() const
    {
        if(!m_controller) {
            throw std::logic_error("future is invalid");
        }

        return m_controller->get();
    }

    template<typename ThreadPool, typename T>
    template<typename Func>
    auto future<ThreadPool, T>::then(Func &&task)
    {
        using ret_t = function_ret_t<Func>;
        promise<ThreadPool, ret_t> p(m_thread_pool); //TODO использовать только future здесь

        if constexpr(std::is_void_v<T>) {
            m_controller->set_on_success([controller = p.get_controller(),
                                         task = std::forward<Func>(task)]() mutable {
                try {
                    static_assert(function_arg_count_v<Func> == 0,
                                  "callback must have 0 argument");

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
        } else {
            m_controller->set_on_success([controller = p.get_controller(),
                                         task = std::forward<Func>(task)]
                                         (add_lvalue_ref_to_value_t<T> val) mutable {
                try {
                    static_assert(function_arg_count_v<Func> == 1,
                                  "callback must have 1 argument");
                    using arg_t = function_arg_t<0, Func>;
                    static_assert(std::is_same_v<remove_type_qualifiers_t<arg_t>,
                                  remove_type_qualifiers_t<T>>,
                                  "future stored type and callback argument type don't match");

                    if constexpr(!std::is_void_v<ret_t>) {
                        controller->set_value(task(static_cast<arg_t>(val)));
                    } else {
                        task(static_cast<arg_t>(val));
                        controller->set_value();
                    }
                } catch(...) {
                    controller->set_exception(std::current_exception());
                }
            });
        }

        auto fail = [controller = p.get_controller()](std::exception_ptr e) {
            controller->set_exception(e);
        };
        m_controller->set_on_fail_if_not_set(std::move(fail));

        auto f = p.get_future();

        if constexpr(is_future_v<ret_t> && is_value_v<ret_t>) {
            using future_t = ret_t;
            using future_store = future_store_type_t<future_t>;

            promise<ThreadPool, future_store> next_promise(m_thread_pool);
            auto next_future = next_promise.get_future();
            auto next_controller = next_promise.get_controller();

            auto c = p.get_controller();
            c->set_on_success([next_controller]([[maybe_unused]] future_t &&val) {
                auto controller = val.get_controller();
                if constexpr(std::is_void_v<future_store>) {
                    controller->set_on_success([next_controller]() {
                        next_controller->set_value();
                    });
                } else {
                    controller->set_on_success([next_controller](future_store &&v) {
                        next_controller->set_value(std::forward<future_store>(v));
                    });
                }

                controller->set_on_fail_if_not_set([next_controller](std::exception_ptr e) {
                    next_controller->set_exception(e);
                });
            });

            c->set_on_fail_if_not_set([next_controller](std::exception_ptr e) {
                next_controller->set_exception(e);
            });

            return next_future;
        } else {
            return f;
        }
    }

    template<typename ThreadPool, typename T>
    void future<ThreadPool, T>::catched(
            std::function<void(std::exception_ptr)> &&task) &
    {
        m_controller->set_on_fail(std::move(task));
    }

    template<typename ThreadPool, typename T>
    future<ThreadPool, T> future<ThreadPool, T>::catched(
                                     std::function<void(std::exception_ptr)> &&task) &&
    {
        m_controller->set_on_fail(std::move(task));
        return std::move(*this);
    }

    template<typename ThreadPool, typename T>
    bool future<ThreadPool, T>::is_valid() const
    {
        return m_controller != nullptr;
    }

    template<typename ThreadPool, typename T>
    auto future<ThreadPool, T>::get_controller() const
    {
        return m_controller;
    }
}
