#pragma once
#include "is-future.h"
#include "future-store-type-or-self.h"
#include "is-future-tuple.h"

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
        auto new_controller = future_controller<ret_t, ThreadPool>::create(m_thread_pool);

        if constexpr(std::is_void_v<T>) {
            m_controller->set_on_success([new_controller,
                                         task = std::forward<Func>(task)]() mutable {
                try {
                    static_assert(function_arg_count_v<Func> == 0,
                                  "callback must have 0 argument");

                    if constexpr(!std::is_void_v<ret_t>) {
                        new_controller->set_value(task());
                    } else {
                        task();
                        new_controller->set_value();
                    }
                } catch(...) {
                    new_controller->set_exception(std::current_exception());
                }
            });
        } else {
            m_controller->set_on_success([new_controller,
                                         task = std::forward<Func>(task)]
                                         (add_lvalue_ref_to_value_t<T> val) mutable {
                try {
                    if constexpr(is_future_tuple_v<T>) {
                        static_assert(is_value_v<T>);

                        if constexpr(!std::is_void_v<ret_t>) {
                            new_controller->set_value(std::apply([&task](auto&&...args) -> decltype(auto) {
                                return task(std::forward<decltype(args)>(args)...);
                            }, std::move(val.to_underlying())));
                        } else {
                            std::apply([&task](auto&&...args) {
                                task(std::forward<decltype(args)>(args)...);
                            }, std::move(val.to_underlying()));
                            new_controller->set_value();
                        }

                    } else {
                        static_assert(function_arg_count_v<Func> == 1,
                                      "callback must have 1 argument");
                        using arg_t = function_arg_t<0, Func>;
                        static_assert(std::is_same_v<remove_type_qualifiers_t<arg_t>,
                                      remove_type_qualifiers_t<T>>,
                                      "future stored type and callback argument type don't match");

                        if constexpr(!std::is_void_v<ret_t>) {
                            new_controller->set_value(task(static_cast<arg_t>(val)));
                        } else {
                            task(static_cast<arg_t>(val));
                            new_controller->set_value();
                        }
                    }
                } catch(...) {
                    new_controller->set_exception(std::current_exception());
                }
            });
        }

        auto fail = [new_controller](std::exception_ptr e) {
            new_controller->set_exception(e);
        };
        m_controller->set_on_fail_if_not_set(std::move(fail));

        if constexpr(is_future_v<ret_t>) {
            static_assert(is_value_v<ret_t>);

            using future_t = ret_t;
            using future_store = future_store_type_or_self_t<future_t>;

            auto next_controller2 = future_controller<future_store, ThreadPool>::create(m_thread_pool);
            new_controller->set_on_success([next_controller2]([[maybe_unused]] future_t &&val) {
                auto controller = val.get_controller();
                if constexpr(std::is_void_v<future_store>) {
                    controller->set_on_success([next_controller2]() {
                        next_controller2->set_value();
                    });
                } else {
                    controller->set_on_success([next_controller2](future_store &&v) {
                        next_controller2->set_value(std::forward<future_store>(v));
                    });
                }

                controller->set_on_fail_if_not_set([next_controller2](std::exception_ptr e) {
                    next_controller2->set_exception(e);
                });
            });

            new_controller->set_on_fail_if_not_set([next_controller2](std::exception_ptr e) {
                next_controller2->set_exception(e);
            });

            return future<ThreadPool, future_store>(m_thread_pool, std::move(next_controller2));
        } else {
            return future<ThreadPool, ret_t>(m_thread_pool, std::move(new_controller));
        }
    }

    template<typename ThreadPool, typename T>
    future<ThreadPool, T> &future<ThreadPool, T>::catched(
            std::function<void(std::exception_ptr)> &&task) &
    {
        m_controller->set_on_fail(std::move(task));
        return *this;
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
    auto future<ThreadPool, T>::get_controller() const noexcept
    {
        return m_controller;
    }
}
