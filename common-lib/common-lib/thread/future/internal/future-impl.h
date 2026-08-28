#pragma once
#include "is-future.h"
#include "future-store-type-or-self.h"
#include "is-future-tuple.h"

#include <common-lib/mpl/type-transform.h>
#include <common-lib/mpl/type-traits.h>

#include <common-lib/utils/type-qualifiers-cast.h>
#include "future.h"

#include <functional>

namespace vshalygin::cl::internal {
    template<typename ThreadPool, typename T>
    future<ThreadPool, T>::future(
                         ThreadPool *thread_pool,
                         std::shared_ptr<future_controller<ThreadPool, T>> controller)
        : m_thread_pool(thread_pool)
        , m_controller(std::move(controller))
    {
        assert(m_thread_pool);
    }

    template<typename ThreadPool, typename T>
    template<typename U, std::enable_if_t<!std::is_void_v<U>, int>>
    future<ThreadPool, T>::future(ThreadPool *thread_pool, U &&val)
        : m_thread_pool(thread_pool)
        , m_controller(std::make_shared<future_controller<ThreadPool, T>>(thread_pool))
    {
        assert(m_thread_pool);
        m_controller->set_self_shared_ptr(m_controller);

        m_controller->set_value(std::forward<U>(val));
    }

    template<typename ThreadPool, typename T>
    template<typename U, std::enable_if_t<std::is_void_v<U>, int>>
    future<ThreadPool, T>::future(ThreadPool *thread_pool)
        : m_thread_pool(thread_pool)
        , m_controller(std::make_shared<future_controller<ThreadPool, void>>(thread_pool))
    {
        assert(m_thread_pool);
        m_controller->set_self_shared_ptr(m_controller);

        m_controller->set_value();
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
    void future<ThreadPool, T>::wait() const
    {
        if(!m_controller) {
            throw std::logic_error("future is invalid");
        }

        return m_controller->wait();
    }

    template<typename ThreadPool, typename T>
    bool future<ThreadPool, T>::wait_for(std::chrono::milliseconds timeout) const
    {
        if(!m_controller) {
            throw std::logic_error("future is invalid");
        }

        return m_controller->wait_for(timeout);
    }

    template<typename ThreadPool, typename T>
    template<typename U, typename ControllerSp>
    std::shared_ptr<future_controller<ThreadPool, U>>
        future<ThreadPool, T>::create_child_controller(ControllerSp controller)
    {
        auto next_controller_temp = std::make_unique<future_controller<ThreadPool, U>>(m_thread_pool);
        auto next_controller_ptr = next_controller_temp.get();
        controller->add_child(std::move(next_controller_temp));
        auto next_controller =
            std::shared_ptr<future_controller<ThreadPool, U>>(controller, next_controller_ptr);
        next_controller->set_self_shared_ptr(next_controller);

        return next_controller;
    }

    template<typename ThreadPool, typename T>
    template<typename Future, typename Controller>
    auto future<ThreadPool, T>::flatten_future(std::shared_ptr<Controller> controller)
    {
        static_assert(is_flattenable_future_v<Future>);

        using future_t = Future;
        using future_store = future_store_type_or_self_t<future_t>;

        auto next_controller = create_child_controller<future_store>(controller);
        auto next_controller_wp = std::weak_ptr(next_controller);

        auto on_success = [next_controller_wp](fvalue<ThreadPool, future_t> value) {
            assert(!next_controller_wp.expired());
            auto next_controller = std::shared_ptr(next_controller_wp);
            future_t val(std::move(*value.lock()));

            if(!val.is_valid()) {
                next_controller->set_exception(std::make_exception_ptr(
                    std::logic_error("returned future is invalid")));
                return;
            }

            auto controller = val.get_controller();
            val = {};

            controller->add_dependent(next_controller);

            auto on_fail = [next_controller_wp](std::exception_ptr e) {
                assert(!next_controller_wp.expired());
                std::shared_ptr(next_controller_wp)->set_exception(e);
            };

            if constexpr(std::is_void_v<future_store>) {
                auto on_success = [next_controller_wp]() {
                    assert(!next_controller_wp.expired());
                    std::shared_ptr(next_controller_wp)->set_value();
                };
                controller->set_on_success_and_fail(std::move(on_success), std::move(on_fail));
            } else {
                auto on_success = [next_controller_wp](fvalue<ThreadPool, future_store> value) {
                    assert(!next_controller_wp.expired());
                    std::shared_ptr(next_controller_wp)->set_value_state(
                        value.get_value_state());
                };
                controller->set_on_success_and_fail(std::move(on_success), std::move(on_fail));
            }
        };
        auto on_fail = [next_controller_wp](std::exception_ptr e) {
            assert(!next_controller_wp.expired());
            std::shared_ptr(next_controller_wp)->set_exception(e);
        };

        controller->set_on_success_and_fail(std::move(on_success), std::move(on_fail));

        return future<ThreadPool, future_store>(m_thread_pool, std::move(next_controller));
    }

    template<typename ThreadPool, typename T>
    template<typename Func, typename ControllerSp,
             typename U, std::enable_if_t<!std::is_void_v<U>, int>>
    void future<ThreadPool, T>::exec_then_on_success(ControllerSp controller,
                                                     Func &&task,
                                                     fvalue<ThreadPool, U> value)
    {
        using ret_t = then_result_t<ThreadPool, U, Func>;

        try {
            if constexpr(!std::is_void_v<ret_t>) {
                if constexpr(std::is_reference_v<ret_t>) {
                    auto value_state = value.get_value_state();
                    decltype(auto) result =
                        std::invoke(std::forward<Func>(task), std::move(value));
                    controller->set_value(
                        std::forward<ret_t>(result),
                        std::move(value_state));
                } else {
                    controller->set_value(
                        std::invoke(std::forward<Func>(task), std::move(value)));
                }
            } else {
                std::invoke(std::forward<Func>(task), std::move(value));
                controller->set_value();
            }
        } catch(...) {
            controller->set_exception(std::current_exception());
        }
    }

    template<typename ThreadPool, typename T>
    template<typename Func, typename ControllerSp,
              typename Dummy, std::enable_if_t<std::is_void_v<Dummy>, int>>
    void future<ThreadPool, T>::exec_then_on_success(ControllerSp controller,
                                                     Func &&task)
    {
        using ret_t = then_result_t<ThreadPool, T, Func>;

        try {
            static_assert(function_arg_count_v<Func> == 0,
                          "success callback must have 0 argument");

            if constexpr(!std::is_void_v<ret_t>) {
                controller->set_value(task());
            } else {
                task();
                controller->set_value();
            }
        } catch(...) {
            controller->set_exception(std::current_exception());
        }
    }

    template<typename ThreadPool, typename T>
    template<typename Func>
    auto future<ThreadPool, T>::then(Func &&task)
    {
        using ret_t = then_result_t<ThreadPool, T, Func>;

        if(!m_controller) {
            throw std::logic_error("future is invalid");
        }

        if constexpr(is_future_v<ret_t> && !is_flattenable_future_v<ret_t>) {
            static_assert(is_flattenable_future_v<ret_t>,
                          "future callback must return future by value without cv/ref qualifiers");
        } else if constexpr(!is_future_v<ret_t>) {
            auto new_controller = create_child_controller<ret_t>(m_controller);
            auto new_controller_wp = std::weak_ptr(new_controller);

            auto on_fail = [new_controller_wp](std::exception_ptr e) {
                assert(!new_controller_wp.expired());
                std::shared_ptr(new_controller_wp)->set_exception(e);
            };

            if constexpr(std::is_void_v<T>) {
                auto on_success = [new_controller_wp,
                    task = std::forward<Func>(task)]() mutable {
                    assert(!new_controller_wp.expired());
                    exec_then_on_success(std::shared_ptr(new_controller_wp),
                                         std::forward<Func>(task));
                };
                m_controller->set_on_success_and_fail(std::move(on_success), std::move(on_fail));
            } else {
                auto on_success = [new_controller_wp,
                                   task = std::forward<Func>(task)]
                                   (fvalue<ThreadPool, T> value) mutable {
                    assert(!new_controller_wp.expired());
                    exec_then_on_success(std::shared_ptr(new_controller_wp),
                                         std::forward<Func>(task),
                                         std::move(value));
                };
                m_controller->set_on_success_and_fail(std::move(on_success), std::move(on_fail));
            }

            return future<ThreadPool, ret_t>(m_thread_pool, std::move(new_controller));
        } else {
            auto new_controller = create_child_controller<ret_t>(m_controller);
            auto new_controller_wp = std::weak_ptr(new_controller);

            auto on_fail = [new_controller_wp](std::exception_ptr e) {
                assert(!new_controller_wp.expired());
                std::shared_ptr(new_controller_wp)->set_exception(e);
            };

            if constexpr(std::is_void_v<T>) {
                auto on_success = [new_controller_wp,
                    task = std::forward<Func>(task)]() mutable {
                    assert(!new_controller_wp.expired());
                    exec_then_on_success(std::shared_ptr(new_controller_wp),
                                         std::forward<Func>(task));
                };
                m_controller->set_on_success_and_fail(std::move(on_success), std::move(on_fail));
            } else {
                auto on_success = [new_controller_wp,
                                   task = std::forward<Func>(task)]
                                   (fvalue<ThreadPool, T> value) mutable {
                    assert(!new_controller_wp.expired());
                    exec_then_on_success(std::shared_ptr(new_controller_wp),
                                         std::forward<Func>(task),
                                         std::move(value));
                };
                m_controller->set_on_success_and_fail(std::move(on_success), std::move(on_fail));
            }

            return flatten_future<ret_t>(std::move(new_controller));
        }
    }

    template<typename ThreadPool, typename T>
    template<typename Func>
    auto future<ThreadPool, T>::catched(Func &&task)
    {
        using ret_t = function_ret_t<Func>;

        static_assert(function_arg_count_v<Func> == 1 &&
                      std::is_same_v<function_arg_t<0, Func>, std::exception_ptr>,
                      "fail callback argument must be std::exception_ptr");

        if constexpr(is_future_v<ret_t> && !is_flattenable_future_v<ret_t>) {
            static_assert(is_flattenable_future_v<ret_t>,
                          "future callback must return future by value without cv/ref qualifiers");
        } else {
            static_assert(std::is_same_v<future_store_type_or_self_t<ret_t>, T> ||
                          std::is_void_v<ret_t>,
                          "fail callback argument must return future storing type or void");

            if(!m_controller) {
                throw std::logic_error("future is invalid");
            }

            auto new_controller =
                create_child_controller<future_store_type_or_self_t<ret_t>>(m_controller);
            auto new_controller_wp = std::weak_ptr(new_controller);

            auto on_success =
                get_on_success_for_catched_method<Func, decltype(new_controller_wp)>(new_controller_wp);
            auto on_fail =
                get_on_fail_for_catched_method<Func, decltype(new_controller_wp)>(std::forward<Func>(task),
                                                                                  new_controller_wp);

            m_controller->set_on_success_and_fail(std::move(on_success), std::move(on_fail));

            return future<ThreadPool, future_store_type_or_self_t<ret_t>>(
                m_thread_pool,
                std::move(new_controller));
        }
    }

    template<typename ThreadPool, typename T>
    template<typename Func>
    auto future<ThreadPool, T>::finally(Func &&task)
    {
        using ret_t = function_ret_t<Func>;

        static_assert(function_arg_count_v<Func> == 0,
                      "finally callback must have no argument");

        if constexpr(is_future_v<ret_t> && !is_flattenable_future_v<ret_t>) {
            static_assert(is_flattenable_future_v<ret_t>,
                          "future callback must return future by value without cv/ref qualifiers");
        } else {
            static_assert(std::is_same_v<ret_t, void> ||
                          std::is_same_v<ret_t, future<ThreadPool, void>>,
                          "finally callback must return void or future storing void type");

            if(!m_controller) {
                throw std::logic_error("future is invalid");
            }

            auto new_controller = create_child_controller<T>(m_controller);
            auto new_controller_wp = std::weak_ptr(new_controller);
            auto task_sp =
                std::make_shared<remove_type_qualifiers_t<Func>>(std::forward<Func>(task));

            auto on_success = get_on_success_for_finally_method(task_sp, new_controller_wp);
            auto on_fail = get_on_fail_for_finally_method(task_sp, new_controller_wp);
            m_controller->set_on_success_and_fail(std::move(on_success), std::move(on_fail));

            return future<ThreadPool, T>(m_thread_pool, std::move(new_controller));
        }
    }

    template<typename ThreadPool, typename T>
    bool future<ThreadPool, T>::has_value() const
    {
        if(!m_controller) {
            throw std::logic_error("future is invalid");
        }

        return m_controller->has_value();
    }

    template<typename ThreadPool, typename T>
    bool future<ThreadPool, T>::has_exception() const
    {
        if(!m_controller) {
            throw std::logic_error("future is invalid");
        }

        return m_controller->has_exception();
    }

    template<typename ThreadPool, typename T>
    bool future<ThreadPool, T>::is_valid() const noexcept
    {
        return m_controller != nullptr;
    }

    template<typename ThreadPool, typename T>
    auto future<ThreadPool, T>::get_controller() const noexcept
    {
        return m_controller;
    }

    template<typename ThreadPool, typename T>
    template<typename Func, typename NewControllerWp>
    auto future<ThreadPool, T>::get_on_success_for_catched_method(NewControllerWp new_controller_wp) const
    {
        using ret_t = function_ret_t<Func>;

        if constexpr(std::is_void_v<T>) {
            return [new_controller_wp]() {
                assert(!new_controller_wp.expired());
                std::shared_ptr(new_controller_wp)->set_value();
            };
        } else if constexpr(std::is_void_v<ret_t>) {
            return [new_controller_wp](fvalue<ThreadPool, T> /*ignore_value*/) mutable {
                assert(!new_controller_wp.expired());
                std::shared_ptr(new_controller_wp)->set_value();
            };
        } else {
            return [new_controller_wp](fvalue<ThreadPool, T> value) {
                assert(!new_controller_wp.expired());
                std::shared_ptr(new_controller_wp)->set_value_state(
                    value.get_value_state());
            };
        }
    }

    template<typename ThreadPool, typename T>
    template<typename Func, typename NewControllerWp>
    auto future<ThreadPool, T>::get_on_fail_for_catched_method(Func &&task, NewControllerWp new_controller_wp) const
    {
        using ret_t = function_ret_t<Func>;

        if constexpr(!is_flattenable_future_v<ret_t>) {
            return [new_controller_wp, task = std::forward<Func>(task)](std::exception_ptr ep) mutable {
                try {
                    if constexpr(!std::is_void_v<ret_t>) {
                        assert(!new_controller_wp.expired());
                        std::shared_ptr(new_controller_wp)->set_value(task(ep));
                    } else {
                        assert(!new_controller_wp.expired());
                        task(ep);
                        std::shared_ptr(new_controller_wp)->set_value();
                    }
                } catch(...) {
                    assert(!new_controller_wp.expired());
                    std::shared_ptr(new_controller_wp)->set_exception(std::current_exception());
                }
            };
        } else {
            static_assert(is_value_v<ret_t>);
            using future_t = ret_t;
            using future_store = future_store_type_or_self_t<future_t>;

            return [new_controller_wp, task = std::forward<Func>(task)](std::exception_ptr ep) mutable {
                try {
                    auto future = task(ep);
                    if(!future.is_valid()) {
                        throw std::logic_error("returned future is invalid");
                    }

                    auto future_controller = future.get_controller();
                    std::shared_ptr new_controller(new_controller_wp);
                    future_controller->add_dependent(new_controller);

                    auto on_fail = [new_controller_wp](std::exception_ptr ep) {
                        assert(!new_controller_wp.expired());
                        std::shared_ptr(new_controller_wp)->set_exception(ep);
                    };

                    if constexpr(std::is_void_v<future_store>) {
                        auto on_success = [new_controller_wp]() {
                            assert(!new_controller_wp.expired());
                            std::shared_ptr(new_controller_wp)->set_value();
                        };
                        future_controller->set_on_success_and_fail(std::move(on_success), std::move(on_fail));
                    } else {
                        auto on_success = [new_controller_wp](fvalue<ThreadPool, future_store> value) {
                            assert(!new_controller_wp.expired());
                            std::shared_ptr(new_controller_wp)->set_value_state(
                                value.get_value_state());
                        };

                        future_controller->set_on_success_and_fail(std::move(on_success), std::move(on_fail));
                    }
                } catch(...) {
                    assert(!new_controller_wp.expired());
                    std::shared_ptr(new_controller_wp)->set_exception(std::current_exception());
                }
            };
        }
    }

    template<typename ThreadPool, typename T>
    template<typename FuncSp, typename NewControllerWp>
    auto future<ThreadPool, T>::get_on_success_for_finally_method(FuncSp task_sp,
                                                                  NewControllerWp new_controller_wp) const
    {
        using ret_t = function_ret_t<decltype(*task_sp)>;

        if constexpr(std::is_void_v<T>) {
            return [new_controller_wp, task_sp]() mutable {
                assert(!new_controller_wp.expired());
                std::shared_ptr new_controller(new_controller_wp);
                try {
                    if constexpr(std::is_void_v<ret_t>) {
                        (*task_sp)();
                        new_controller->set_value();
                    } else {
                        static_assert(is_flattenable_future_v<ret_t>);
            
                        auto future = (*task_sp)();
                        if(!future.is_valid()) {
                            throw std::logic_error("returned future is invalid");
                        }

                        auto future_controller = future.get_controller();
                        future_controller->add_dependent(new_controller);

                        auto on_success = [new_controller_wp]() {
                            assert(!new_controller_wp.expired());
                            std::shared_ptr(new_controller_wp)->set_value();
                        };
                        auto on_fail = [new_controller_wp](std::exception_ptr ep) {
                            assert(!new_controller_wp.expired());
                            std::shared_ptr(new_controller_wp)->set_exception(ep);
                        };

                        future_controller->set_on_success_and_fail(std::move(on_success), std::move(on_fail));
                    }
                } catch(...) {
                    new_controller->set_exception(std::current_exception());
                }
            };
        } else {
            return [new_controller_wp, task_sp] (fvalue<ThreadPool, T> value) mutable {
                assert(!new_controller_wp.expired());
                std::shared_ptr new_controller(new_controller_wp);
                auto value_state = value.get_value_state();
                try {
                    if constexpr(std::is_void_v<ret_t>) {
                        (*task_sp)();
                        new_controller->set_value_state(std::move(value_state));
                    } else {
                        static_assert(is_flattenable_future_v<ret_t>);
            
                        auto future = (*task_sp)();
                        if(!future.is_valid()) {
                            throw std::logic_error("returned future is invalid");
                        }

                        auto future_controller = future.get_controller();
                        future_controller->add_dependent(new_controller);

                        auto on_success = [new_controller_wp, value_state = std::move(value_state)]() mutable {
                            assert(!new_controller_wp.expired());
                            std::shared_ptr(new_controller_wp)->set_value_state(std::move(value_state));
                        };
                        auto on_fail = [new_controller_wp](std::exception_ptr ep) {
                            assert(!new_controller_wp.expired());
                            std::shared_ptr(new_controller_wp)->set_exception(ep);
                        };
                        future_controller->set_on_success_and_fail(std::move(on_success), std::move(on_fail));
                    }
                } catch(...) {
                    new_controller->set_exception(std::current_exception());
                }
            };
        }
    }

    template<typename ThreadPool, typename T>
    template<typename FuncSp, typename NewControllerWp>
    auto future<ThreadPool, T>::get_on_fail_for_finally_method(FuncSp task_sp,
                                                               NewControllerWp new_controller_wp) const
    {
        using ret_t = function_ret_t<decltype(*task_sp)>;

        return [new_controller_wp, task_sp] (std::exception_ptr ep) mutable {
            assert(!new_controller_wp.expired());
            std::shared_ptr new_controller(new_controller_wp);
            try {
                if constexpr(std::is_void_v<ret_t>) {
                    (*task_sp)();
                    new_controller->set_exception(ep);
                } else {
                    static_assert(is_flattenable_future_v<ret_t>);

                    auto future = (*task_sp)();
                    if(!future.is_valid()) {
                        throw std::logic_error("returned future is invalid");
                    }

                    auto future_controller = future.get_controller();
                    future_controller->add_dependent(new_controller);

                    auto on_success = [new_controller_wp, ep]() mutable {
                        assert(!new_controller_wp.expired());
                        std::shared_ptr(new_controller_wp)->set_exception(ep);
                    };
                    auto on_fail = [new_controller_wp](std::exception_ptr ep) {
                        assert(!new_controller_wp.expired());
                        std::shared_ptr(new_controller_wp)->set_exception(ep);
                    };
                    future_controller->set_on_success_and_fail(std::move(on_success), std::move(on_fail));
                }
            } catch(...) {
                new_controller->set_exception(std::current_exception());
            }
        };
    }
}
