#pragma once
#include "future-controller.h"

namespace vshalygin::cl::internal {
    template<typename Ret>
    class ipromise_function
    {
    public:
        virtual ~ipromise_function() = default;

        virtual Ret call() = 0;
    };

    template<typename Func>
    class promise_function
        : public ipromise_function<function_ret_t<Func>>
    {
    public:
        template<typename F> //TODO check convertible static
        explicit promise_function(F &&func)
            : m_func(std::forward<F>(func))
        {
            static_assert(std::is_constructible_v<Func, F>);
        }

        promise_function(const promise_function &) = delete;
        promise_function &operator=(const promise_function &) = delete;

        function_ret_t<Func> call() override
        {
            return m_func();
        }

    private:
        remove_type_qualifiers_t<Func> m_func;
    };

    template<typename T, typename ThreadPool>
    class promise_impl;

    template<typename T, typename ThreadPool>
    class future_impl
    {
        friend class promise_impl<T, ThreadPool>;

        explicit future_impl(
                        ThreadPool *thread_pool,
                        std::shared_ptr<future_controller<T, ThreadPool>> controller);

    public:
        future_impl() = default;
        future_impl(const future_impl &) = delete;
        future_impl &operator=(const future_impl &) = delete;
        future_impl(future_impl &&) = default;
        future_impl &operator=(future_impl &&) = default;

        const T &get() const;
        [[nodiscard]] T extract();

        template<typename Func>
        future_impl<function_ret_t<Func>, ThreadPool> then(Func &&task);

        future_impl<T, ThreadPool> &catched(
                           std::function<void(std::exception_ptr)> &&task) &;
        future_impl<T, ThreadPool> catched(
            std::function<void(std::exception_ptr)> &&task) &&;

        bool is_valid() const;

    private:
        ThreadPool *m_thread_pool = nullptr;
        std::shared_ptr<future_controller<T, ThreadPool>> m_controller;
    };

    template<typename T, typename ThreadPool>
    class promise_impl
    {
        template<typename U, typename TP>
        friend class promise_impl;

        template<typename U, typename TP>
        friend class future_impl;

        explicit promise_impl(ThreadPool *thread_pool);

    public:
        promise_impl() = default;

        template<typename Function>
        explicit promise_impl(ThreadPool *thread_pool,
                              Function &&function);

        promise_impl(const promise_impl &) = delete;
        promise_impl &operator=(const promise_impl &) = delete;

        promise_impl(promise_impl &&) = default;
        promise_impl &operator=(promise_impl &&) = default;

        future_impl<T, ThreadPool> resolve();

        bool is_valid() const;

    private:
        std::shared_ptr<future_controller<T, ThreadPool>> get_controller() const;

    private:
        ThreadPool *m_thread_pool = nullptr;

        //shared_ptr for thread pools, which don't accept move-only functors
        std::shared_ptr<ipromise_function<T>> m_function;

        std::shared_ptr<future_controller<T, ThreadPool>> m_controller;
    };

    template<typename T, typename ThreadPool>
    future_impl<T, ThreadPool>::future_impl(
                         ThreadPool *thread_pool,
                         std::shared_ptr<future_controller<T, ThreadPool>> controller)
        : m_thread_pool(thread_pool)
        , m_controller(std::move(controller))
    {
        assert(m_thread_pool);
    }

    template<typename T, typename ThreadPool>
    const T &future_impl<T, ThreadPool>::get() const
    {
        if(!m_controller) {
            throw std::logic_error("future_impl is invalid");
        }
    
        return m_controller->get();
    }

    template<typename T, typename ThreadPool>
    T future_impl<T, ThreadPool>::extract()
    {
        if(!m_controller) {
            throw std::logic_error("future_impl is invalid");
        }

        return m_controller->extract();
    }

    template<typename T, typename ThreadPool>
    template<typename Func>
    future_impl<function_ret_t<Func>, ThreadPool>
        future_impl<T, ThreadPool>::then(Func &&task)
    {
        using ret_t = function_ret_t<Func>;

        promise_impl<ret_t, ThreadPool> promise(m_thread_pool);
        future_impl<function_ret_t<Func>, ThreadPool> future(m_thread_pool,
                                                             promise.get_controller());
        auto success = [controller = promise.get_controller(),
                        task = std::forward<Func>(task)](T &&val) mutable {
            try {
                controller->set_value(task(std::forward<T &&>(val)));
            } catch(...) {
                controller->set_exception(std::current_exception());
            }
        };
        auto fail = [controller = promise.get_controller()](std::exception_ptr e) {
            controller->set_exception(e);
        };
        m_controller->set_on_success(std::move(success));
        m_controller->set_on_fail_if_not_set(std::move(fail));

        return future;
    }

    template<typename T, typename ThreadPool>
    future_impl<T, ThreadPool> &
        future_impl<T, ThreadPool>::catched(
            std::function<void(std::exception_ptr)> &&task) &
    {
        m_controller->set_on_fail(std::move(task));
        return *this;
    }

    template<typename T, typename ThreadPool>
    future_impl<T, ThreadPool> future_impl<T, ThreadPool>::catched(
        std::function<void(std::exception_ptr)> &&task) &&
    {
        m_controller->set_on_fail(std::move(task));
        return std::move(*this);
    }

    template<typename T, typename ThreadPool>
    bool future_impl<T, ThreadPool>::is_valid() const
    {
        return m_controller != nullptr;
    }

    template<typename T, typename ThreadPool>
    promise_impl<T, ThreadPool>::promise_impl(ThreadPool *thread_pool)
        : m_thread_pool(thread_pool)
        , m_controller(std::make_shared<future_controller<T, ThreadPool>>(thread_pool))
    {
        assert(m_thread_pool);
    }

    template<typename T, typename ThreadPool>
    template<typename Function>
    promise_impl<T, ThreadPool>::promise_impl(ThreadPool *thread_pool,
                                              Function &&function)
        : m_thread_pool(thread_pool)
        , m_function(std::make_shared<promise_function<Function>>
                           (std::forward<Function>(function)))
        , m_controller(std::make_shared<future_controller<T, ThreadPool>>(thread_pool))
    {
        static_assert(function_arg_count_v<Function> == 0);
        assert(m_thread_pool);
    }

    template<typename T, typename ThreadPool>
    future_impl<T, ThreadPool> promise_impl<T, ThreadPool>::resolve()
    {
        if(!m_function) {
            throw std::logic_error("no resolve function");
        }

        m_thread_pool->post([controller = m_controller,
                             func = std::move(m_function)]() mutable {
            try {
                controller->set_value(func->call());
            } catch (...) {
                controller->set_exception(std::current_exception());
            }
        });

        return future_impl<T, ThreadPool>(m_thread_pool, m_controller);
    }

    template<typename T, typename ThreadPool>
    bool promise_impl<T, ThreadPool>::is_valid() const
    {
        return m_controller != nullptr;
    }

    template<typename T, typename ThreadPool>
    std::shared_ptr<future_controller<T, ThreadPool>>
        promise_impl<T, ThreadPool>::get_controller() const
    {
        return m_controller;
    }
}
