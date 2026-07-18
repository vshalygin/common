#pragma once
#include "is-future.h"
#include "future-store-type-or-self.h"
#include "is-future-tuple.h"

#include <common-lib/mpl/type-transform.h>
#include <common-lib/mpl/type-traits.h>

#include <common-lib/utils/type-qualifiers-cast.h>

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
        m_controller->set_self_shared_ptr(m_controller);
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
    template<typename U>
    std::shared_ptr<future_controller<ThreadPool, U>>
        future<ThreadPool, T>::create_child_controller(auto controller)
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
    template<typename U>
    std::shared_ptr<future_controller<ThreadPool, U>> future<ThreadPool, T>::create_controller()
    {
        auto controller = std::make_shared<future_controller<ThreadPool, U>>(m_thread_pool);
        controller->set_self_shared_ptr(controller);

        return controller;
    }

    template<typename ThreadPool, typename T>
    template<typename Future>
    auto future<ThreadPool, T>::flatten_future(auto controller)
    {
        static_assert(is_future_v<Future>);
        static_assert(is_value_v<Future>);

        using future_t = Future;
        using future_store = future_store_type_or_self_t<future_t>;

        auto next_controller = create_child_controller<future_store>(m_controller);

        controller->set_on_success([next_controller_wp = std::weak_ptr(next_controller)](future_t &&val) {
            auto controller = val.get_controller();
            if constexpr(std::is_void_v<future_store>) {
                controller->set_on_success([next_controller_wp]() {
                    assert(!next_controller_wp.expired());
                    std::shared_ptr(next_controller_wp)->set_value();
                });
            } else {
                controller->set_on_success([next_controller_wp](future_store &&v) {
                    assert(!next_controller_wp.expired());
                    std::shared_ptr(next_controller_wp)->set_value(std::forward<future_store>(v));
                });
            }

            controller->set_on_fail([next_controller_wp](std::exception_ptr e) {
                assert(!next_controller_wp.expired());
                std::shared_ptr(next_controller_wp)->set_exception(e);
            });
        });

        controller->set_on_fail([next_controller_wp = std::weak_ptr(next_controller)](std::exception_ptr e) {
            assert(!next_controller_wp.expired());
            std::shared_ptr next_controller(next_controller_wp);

            next_controller->set_exception(e);
        });

        return future<ThreadPool, future_store>(m_thread_pool, std::move(next_controller));
    }

    template<typename ThreadPool, typename T>
    template<typename Func,
             typename U, std::enable_if_t<!std::is_void_v<U>, int>>
    void future<ThreadPool, T>::exec_then_on_success(auto controller,
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
    template<typename Func,
              typename Dummy, std::enable_if_t<std::is_void_v<Dummy>, int>>
    void future<ThreadPool, T>::exec_then_on_success(auto controller,
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

            if constexpr(std::is_void_v<T>) {
                m_controller->set_on_success([new_controller_wp = std::weak_ptr(new_controller),
                                             task = std::forward<Func>(task)]() mutable {
                    assert(!new_controller_wp.expired());
                    exec_then_on_success(std::shared_ptr(new_controller_wp), std::forward<Func>(task));
                });
            } else {
                m_controller->set_on_success([new_controller_wp = std::weak_ptr(new_controller),
                                             task = std::forward<Func>(task)]
                                             (add_lvalue_ref_to_value_t<T> val) mutable {
                    assert(!new_controller_wp.expired());
                    exec_then_on_success(std::shared_ptr(new_controller_wp),
                                         std::forward<Func>(task),
                                         std::forward<decltype(val)>(val));
                });
            }

            m_controller->set_on_fail([new_controller_wp = std::weak_ptr(new_controller)](std::exception_ptr e) {
                assert(!new_controller_wp.expired());
                std::shared_ptr(new_controller_wp)->set_exception(e);
            });

            return future<ThreadPool, ret_t>(m_thread_pool, std::move(new_controller));
        } else {
            auto new_controller = create_controller<ret_t>();

            if constexpr(std::is_void_v<T>) {
                m_controller->set_on_success([new_controller,
                                             task = std::forward<Func>(task)]() mutable {
                    exec_then_on_success(new_controller, std::forward<Func>(task));
                });
            } else {
                m_controller->set_on_success([new_controller,
                                             task = std::forward<Func>(task)]
                                             (add_lvalue_ref_to_value_t<T> val) mutable {
                    exec_then_on_success(new_controller,
                                         std::forward<Func>(task),
                                         std::forward<decltype(val)>(val));
                });
            }

            m_controller->set_on_fail([new_controller](std::exception_ptr e) {
                new_controller->set_exception(e);
            });

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

        if constexpr(std::is_void_v<T>) {
            m_controller->set_on_success([new_controller_wp = std::weak_ptr(new_controller)]() {
                assert(!new_controller_wp.expired());
                std::shared_ptr(new_controller_wp)->set_value_reference();
            });
        } else if constexpr(std::is_void_v<ret_t>) {
            m_controller->set_on_success([new_controller_wp = std::weak_ptr(new_controller)]
                                         (add_lvalue_ref_to_value_t<T> /*ignore_value*/) mutable {
                assert(!new_controller_wp.expired());
                std::shared_ptr(new_controller_wp)->set_value_reference();
            });
        } else {
            auto &mtx = m_controller->get_value_mtx();
            m_controller->set_on_success([new_controller_wp = std::weak_ptr(new_controller), &mtx]
                                         (add_lvalue_ref_to_value_t<T> v) {
                assert(!new_controller_wp.expired());
                std::shared_ptr(new_controller_wp)->set_value_reference(mtx, std::forward<decltype(v)>(v));
            });
        }

        if constexpr(!is_future_v<ret_t>) {
            m_controller->set_on_fail([new_controller_wp = std::weak_ptr(new_controller),
                                      task = std::forward<Func>(task)](std::exception_ptr ep) mutable {
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
            });
        } else {
            static_assert(is_value_v<ret_t>);
            using future_t = ret_t;
            using future_store = future_store_type_or_self_t<future_t>;

            m_controller->set_on_fail([new_controller_wp = std::weak_ptr(new_controller),
                                      task = std::forward<Func>(task)](std::exception_ptr ep) mutable {
                try {
                    auto future = task(ep);
                    auto future_controller = future.get_controller();
                    std::shared_ptr new_controller(new_controller_wp);

                    if constexpr(std::is_void_v<future_store>) {
                        future_controller->set_on_success([new_controller] () {
                            new_controller->set_value();
                        });
                    } else {
                        future_controller->set_on_success([new_controller](future_store &&v) {
                            new_controller->set_value(std::forward<future_store>(v));
                        });
                    }

                    future_controller->set_on_fail([new_controller] (std::exception_ptr ep) {
                        new_controller->set_exception(ep);
                    });
                } catch(...) {
                    assert(!new_controller_wp.expired());
                    std::shared_ptr(new_controller_wp)->set_exception(std::current_exception());
                }
            });
        }

        return future<ThreadPool, future_store_type_or_self_t<ret_t>>(
            m_thread_pool, std::move(new_controller));
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

        auto new_controller = create_child_controller<void>(m_controller);
        m_controller->set_on_finally([new_controller_wp = std::weak_ptr(new_controller),
                                     task = std::forward<Func>(task)]() mutable {
            assert(!new_controller_wp.expired());
            std::shared_ptr new_controller(new_controller_wp);
            try {
                if constexpr(std::is_void_v<ret_t>) {
                    task();
                    new_controller->set_value();
                } else {
                    static_assert(is_future_v<ret_t>);

                    auto future = task();
                    auto future_controller = future.get_controller();
                    future_controller->set_on_success([new_controller]() {
                        new_controller->set_value();
                    });
                    future_controller->set_on_fail([new_controller](std::exception_ptr ep) {
                        new_controller->set_exception(ep);
                    });
                }
            } catch(...) {
                new_controller->set_exception(std::current_exception());
            }
        });

        return future<ThreadPool, void>(m_thread_pool, std::move(new_controller));
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
