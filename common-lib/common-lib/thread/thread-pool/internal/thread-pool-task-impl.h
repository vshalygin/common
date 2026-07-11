#pragma once
#include <common-lib/mpl/type-transform.h>

#include <memory>
#include <stdexcept>

namespace vshalygin::cl::internal {
    template<typename...Args>
    class ithread_pool_func
    {
    public:
        virtual ~ithread_pool_func() = default;

        virtual void call(Args...args) = 0;
        virtual void call(Args...args) const = 0;

        virtual ithread_pool_func<Args...> *copy() const = 0;

        virtual bool is_valid() const = 0;
    };

    template<typename Func, typename...Args>
    class thread_pool_func
        : public ithread_pool_func<Args...>
    {
    public:
        explicit thread_pool_func(Func &&func)
            : m_func(std::move(func))
        {}

        explicit thread_pool_func(const Func &func)
            : m_func(func)
        {}

        void call(Args...args) override
        {
            m_func(std::forward<Args>(args)...);
        }

        void call(Args...args) const override
        {
            m_func(std::forward<Args>(args)...);
        }

        ithread_pool_func<Args...> *copy() const override
        {
            return new thread_pool_func<Func, Args...>(m_func);
        }

        bool is_valid() const override
        {
            if constexpr(std::is_pointer_v<Func> ||
                         std::is_convertible_v<Func, void(*)(Args...)>)
            {
                return m_func != nullptr;
            } else if constexpr(std::is_convertible_v<bool, Func>) {
                return static_cast<bool>(m_func);
            } else {
                return true;
            }
        }

    private:
        mutable Func m_func;
    };

    template<typename... Args>
    class thread_pool_task_proxy final
    {
    public:
        explicit thread_pool_task_proxy(const ithread_pool_func<Args...> *underlying)
            : m_underlying(underlying)
        {}

        template<typename...U>
        void operator()(U&&...args)
        {
            if(!m_underlying) {
                throw std::logic_error("inner function is empty");
            }

            m_underlying->call(std::forward<U>(args)...);
        }

    private:
        const ithread_pool_func<Args...> *m_underlying;
    };

    class thread_pool_task_base
    {
    protected:
        thread_pool_task_base() = default;
    };

    template<typename Signature>
    class thread_pool_task;

    template<typename Ret, typename...Args>
    class thread_pool_task<Ret(Args...)>
    {
        static_assert(sizeof(Ret) == 0, "return type is not void");
    };

    template<typename...Args>
    class thread_pool_task<void(Args...)>
        : private thread_pool_task_base
    {
        using this_type = thread_pool_task<void(Args...)>;
        using ithread_pool_task = ithread_pool_func<Args...>;

    public:
        thread_pool_task() = default;

        template<typename Func,
                 std::enable_if_t<!std::is_same_v<
                               std::remove_cv_t<std::remove_reference_t<Func>>,
                               thread_pool_task<void(Args...)>>, int> = 0>
        explicit thread_pool_task(Func &&func)
            : m_func(std::make_unique<thread_pool_func<std::decay_t<Func>, Args...>>
                      (std::forward<Func>(func)))
        {}

        //TODO remove copying! This functor is designed to execute only in thread_pool and once
        thread_pool_task(const thread_pool_task &other)
        {
            if(other.m_func) {
                m_func = std::unique_ptr<ithread_pool_task>(other.m_func->copy());
            }
        }

        thread_pool_task &operator=(const thread_pool_task &other)
        {
            if(&other == this) {
                return *this;
            }
            if(!other.m_func) {
                m_func.reset();
                return *this;
            }

            m_func.reset(other.m_func->copy());

            return *this;
        }

        thread_pool_task(thread_pool_task &&other) = default;
        thread_pool_task &operator=(thread_pool_task &&) = default;

        bool is_valid() const
        {
            return m_func && m_func->is_valid();
        }

    protected:
        auto create_proxy()
        {
            return thread_pool_task_proxy<Args...>(m_func.get());
        }

        auto create_proxy() const
        {
            return thread_pool_task_proxy<Args...>(m_func.get());
        }

    private:
        std::unique_ptr<ithread_pool_func<Args...>> m_func;
    };

    template<typename T>
    inline constexpr bool is_thread_pool_task_v =
        std::is_base_of_v<thread_pool_task_base, remove_type_qualifiers_t<T>>;
}
