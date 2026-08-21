#pragma once
#include "is-future.h"
#include "future-store-type-or-self.h"
#include "is-future-tuple.h"

#include <common-lib/mpl/type-transform.h>
#include <common-lib/mpl/type-traits.h>

#include <common-lib/utils/type-qualifiers-cast.h>
#include <common-lib/utils/type-wrapper.h>

#include "future.h"

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

        m_controller->set_value(std::forward<T>(val));
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
        static_assert(is_future_v<Future>);
        static_assert(is_value_v<Future>);

        using future_t = Future;
        using future_store = future_store_type_or_self_t<future_t>;

        auto next_controller = create_child_controller<future_store>(controller);
        auto next_controller_wp = std::weak_ptr(next_controller);

        auto on_success = [next_controller_wp](future_t &&val) {
            assert(!next_controller_wp.expired());
            auto next_controller = std::shared_ptr(next_controller_wp);
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
                auto on_success = [next_controller_wp](future_store &&v) {
                    assert(!next_controller_wp.expired());
                    std::shared_ptr(next_controller_wp)->set_value(std::forward<future_store>(v));
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
                                                     add_lvalue_ref_to_value_t<U> param)
    {
        using ret_t = function_ret_t<Func>;

        try {
            if constexpr(is_future_tuple_v<T>) {
                static_assert(is_value_v<T>);

                if constexpr(!std::is_void_v<ret_t>) {
                    controller->set_value(apply(task, param));
                } else {
                    apply(task, param);
                    controller->set_value();
                }
            } else {
                static_assert(function_arg_count_v<Func> == 1,
                              "success callback must have 1 argument");
                using arg_t = function_arg_t<0, Func>;

                if constexpr(!std::is_void_v<ret_t>) {
                    controller->set_value(task(type_qualifiers_cast<arg_t>(param)));
                } else {
                    task(type_qualifiers_cast<arg_t>(param));
                    controller->set_value();
                }
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
        using ret_t = function_ret_t<Func>;

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
        using ret_t = function_ret_t<Func>;

        if constexpr(!is_future_v<ret_t>) {
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
                                   (add_lvalue_ref_to_value_t<T> val) mutable {
                    assert(!new_controller_wp.expired());
                    exec_then_on_success(std::shared_ptr(new_controller_wp),
                                         std::forward<Func>(task),
                                         std::forward<decltype(val)>(val));
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
                                   (add_lvalue_ref_to_value_t<T> val) mutable {
                    assert(!new_controller_wp.expired());
                    exec_then_on_success(std::shared_ptr(new_controller_wp),
                                         std::forward<Func>(task),
                                         std::forward<decltype(val)>(val));
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
        static_assert(std::is_same_v<future_store_type_or_self_t<ret_t>, T> ||
                      std::is_void_v<ret_t>,
                      "fail callback argument must return future storing type or void");

        auto new_controller = create_child_controller<future_store_type_or_self_t<ret_t>>(m_controller);
        auto new_controller_wp = std::weak_ptr(new_controller);

        auto on_success = get_on_success_for_catched_method<Func, decltype(new_controller_wp)>(new_controller_wp);
        auto on_fail = get_on_fail_for_catched_method<Func, decltype(new_controller_wp)>(std::forward<Func>(task),
                                                                                         new_controller_wp);

        m_controller->set_on_success_and_fail(std::move(on_success), std::move(on_fail));

        return future<ThreadPool, future_store_type_or_self_t<ret_t>>(m_thread_pool, std::move(new_controller));
    }

    template<typename ThreadPool, typename T>
    template<typename Func>
    auto future<ThreadPool, T>::finally(Func &&task)
    {
        using ret_t = function_ret_t<Func>;

        static_assert(function_arg_count_v<Func> == 0,
                      "finally callback must have no argument");
        static_assert(std::is_same_v<ret_t, void> || std::is_same_v<ret_t, future<ThreadPool, void>>,
                      "finally callback must return void or future storing void type");

        auto new_controller = create_child_controller<T>(m_controller);
        auto new_controller_wp = std::weak_ptr(new_controller);
        auto task_sp = std::make_shared<remove_type_qualifiers_t<Func>>(std::forward<Func>(task));

        auto on_success = get_on_success_for_finally_method(task_sp, new_controller_wp);
        auto on_fail = get_on_fail_for_finally_method(task_sp, new_controller_wp);
        m_controller->set_on_success_and_fail(std::move(on_success), std::move(on_fail));

        return future<ThreadPool, T>(m_thread_pool, std::move(new_controller));
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
            return [new_controller_wp](add_lvalue_ref_to_value_t<T> /*ignore_value*/) mutable {
                assert(!new_controller_wp.expired());
                std::shared_ptr(new_controller_wp)->set_value();
            };
        } else {
            auto &mtx = m_controller->get_value_mtx_ref();
            return [new_controller_wp, &mtx](add_lvalue_ref_to_value_t<T> v) {
                assert(!new_controller_wp.expired());
                std::shared_ptr(new_controller_wp)->set_value_reference(mtx, std::forward<decltype(v)>(v));
            };
        }
    }

    template<typename ThreadPool, typename T>
    template<typename Func, typename NewControllerWp>
    auto future<ThreadPool, T>::get_on_fail_for_catched_method(Func &&task, NewControllerWp new_controller_wp) const
    {
        using ret_t = function_ret_t<Func>;

        if constexpr(!is_future_v<ret_t>) {
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
                        auto on_success = [new_controller_wp](future_store &&v) {
                            assert(!new_controller_wp.expired());
                            std::shared_ptr(new_controller_wp)->set_value(std::forward<future_store>(v));
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
                        static_assert(is_future_v<ret_t>);
            
                        auto future = (*task_sp)();
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
            auto &mtx = m_controller->get_value_mtx_ref();
            return [new_controller_wp, task_sp, &mtx](add_lvalue_ref_to_value_t<T> val) mutable {
                assert(!new_controller_wp.expired());
                std::shared_ptr new_controller(new_controller_wp);
                try {
                    if constexpr(std::is_void_v<ret_t>) {
                        (*task_sp)();
                        new_controller->set_value_reference(mtx, std::forward<decltype(val)>(val));
                    } else {
                        static_assert(is_future_v<ret_t>);
            
                        auto future = (*task_sp)();
                        auto future_controller = future.get_controller();
                        future_controller->add_dependent(new_controller);

                        type_wrapper<decltype(val)> v(std::forward<decltype(val)>(val));
                        auto on_success = [new_controller_wp, &mtx, v]() mutable {
                            assert(!new_controller_wp.expired());
                            std::shared_ptr(new_controller_wp)->set_value_reference(mtx, v.to_underlying());
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
                    static_assert(is_future_v<ret_t>);

                    auto future = (*task_sp)();
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
