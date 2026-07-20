#pragma once
#include <common-lib/mpl/mpl.h>
#include <common-lib/utils/function.h>
#include <common-lib/thread/thread-pool/thread-pool.h>

#include <stdexcept>
#include <cassert>
#include <tuple>

namespace vshalygin::cl {
    template<typename Signature>
    class thread_pool_task final
    {
        static_assert(std::is_same_v<function_ret_t<Signature>, void>);

        using this_type = thread_pool_task<Signature>;

    public:
        thread_pool_task() = default;

        template<typename Func,
                 std::enable_if_t<!std::is_same_v<remove_type_qualifiers_t<Func>, this_type>, int> = 0>
        thread_pool_task(thread_pool *thread_pool, Func &&func)
            : m_thread_pool(thread_pool)
            , m_func(std::forward<Func>(func))
        {
            assert(m_thread_pool);
            assert(m_func);
        }

        thread_pool_task(const thread_pool_task &other) = delete;
        thread_pool_task &operator=(const thread_pool_task &other) = delete;

        thread_pool_task(thread_pool_task &&other) = default;
        thread_pool_task &operator=(thread_pool_task &&) = default;

        operator bool() const noexcept
        {
            return static_cast<bool>(m_func);
        }

        template<typename...Args>
        void exec(Args&&...args)
        {
            if(!m_func) {
                throw std::runtime_error("no function to execute");
            }
            if(!m_thread_pool) {
                throw std::logic_error("no thread pool");
            }

            m_thread_pool->post([args = std::tuple{ std::forward<Args>(args)... },
                                 func = std::move(m_func)]() mutable {
                std::apply(std::move(func), std::move(args));
            });
        }

    private:
        thread_pool *m_thread_pool;
        function<Signature> m_func;
    };

    template<typename Func>
    thread_pool_task(thread_pool *, Func &&)->thread_pool_task<make_function_type_t<Func>>;
}
