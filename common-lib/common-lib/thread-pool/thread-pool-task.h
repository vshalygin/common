#pragma once
#include <memory>
#include <stdexcept>

namespace vshalygin::cl {
    namespace internal {
        template<typename...Args>
        class ithread_pool_task
        {
        public:
            virtual ~ithread_pool_task() = default;

            virtual void call(Args...args) = 0;
            virtual void call(Args...args) const = 0;

            virtual ithread_pool_task<Args...> *copy() const = 0;
        };

        template<typename Func, typename...Args>
        class thread_pool_task
            : public ithread_pool_task<Args...>
        {
        public:
            explicit thread_pool_task(Func &&func)
                : m_func(std::move(func))
            {}

            explicit thread_pool_task(const Func &func)
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

            ithread_pool_task<Args...> *copy() const override
            {
                return new thread_pool_task<Func, Args...>(m_func);
            }

        private:
            Func m_func;
        };

        template<typename... Args>
        class thread_pool_task_proxy final
        {
        public:
            explicit thread_pool_task_proxy(ithread_pool_task<Args...> *underlying)
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
            ithread_pool_task<Args...> *m_underlying;
        };

        template<typename... Args>
        class thread_pool_task_const_proxy final
        {
        public:
            explicit thread_pool_task_const_proxy(const ithread_pool_task<Args...> *underlying)
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
            const ithread_pool_task<Args...> *m_underlying;
        };

        class thread_pool_task_base
        {
        protected:
            thread_pool_task_base() = default;
        };
    }

    class thread_pool;

    template<typename Signature>
    class thread_pool_task;
    
    template<typename Ret, typename...Args>
    class thread_pool_task<Ret(Args...)>
    {
        static_assert(sizeof(Ret) == 0, "return type is not void");
    };

    template<typename...Args>
    class thread_pool_task<void(Args...)>
        : private internal::thread_pool_task_base
    {
        friend class thread_pool;

        using this_type = thread_pool_task<void(Args...)>;
        using ithread_pool_task = internal::ithread_pool_task<Args...>;

    public:
        template<typename Func,
                 std::enable_if_t<!std::is_same_v<
                                       std::remove_cv_t<std::remove_reference_t<Func>>,
                                       thread_pool_task>,
                                  int> = 0>
        explicit thread_pool_task(Func &&func)
            : m_func(std::make_unique<internal::thread_pool_task<std::decay_t<Func>, Args...>>
                                                            (std::forward<Func>(func)))
        {}

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

    private:
        auto get_proxy()
        {
            return internal::thread_pool_task_proxy<Args...>(m_func.get());
        }

        auto get_proxy() const
        {
            return internal::thread_pool_task_const_proxy<Args...>(m_func.get());
        }

    private:
        std::unique_ptr<internal::ithread_pool_task<Args...>> m_func;
    };

    template<typename T>
    inline constexpr bool is_thread_pool_task_v =
        std::is_base_of_v<
        internal::thread_pool_task_base,
        std::remove_cv_t<std::remove_reference_t<T>>>;
}
