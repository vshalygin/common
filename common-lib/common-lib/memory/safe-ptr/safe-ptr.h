#pragma once
#include <memory>
#include <mutex>
#include <utility>

namespace vsh::cl {
    template<typename T>
    class safe_ptr_proxy
    {
    public:
        safe_ptr_proxy(std::unique_lock<std::recursive_mutex> lock,
                       T *ptr)
            : m_lock(std::move(lock))
            , m_ptr(ptr)
        {}

        safe_ptr_proxy(safe_ptr_proxy &) = delete;
        safe_ptr_proxy &operator=(safe_ptr_proxy &) = delete;

        safe_ptr_proxy(safe_ptr_proxy &&) = delete;
        safe_ptr_proxy &operator=(safe_ptr_proxy &&) = delete;

        T *operator->()
        {
            return m_ptr;
        }

        const T *operator->() const
        {
            return m_ptr;
        }

    private:
        std::unique_lock<std::recursive_mutex> m_lock;
        T *m_ptr;
    };

    template<typename T>
    class safe_ptr
    {
    public:
        safe_ptr() = default;

        template<typename Y>
        explicit safe_ptr(Y *p)
        {
            reset(p);
        }

        //TODO thing about safity here
        safe_ptr(const safe_ptr &) = default;
        safe_ptr& operator=(const safe_ptr &) = default;

        safe_ptr(safe_ptr &&) = default;
        safe_ptr &operator=(safe_ptr &&) = default;

        void reset() noexcept
        {
            m_mval.reset();
        }

        template<typename Y>
        void reset(Y *p)
        {
            m_mval.reset(new mutexed_val{{}, std::unique_ptr<T>(p)});
        }

        void swap(safe_ptr<T> &other)
        {
            m_mval.swap(other.m_mval);
        }

        safe_ptr_proxy<T> operator->()
        {
            std::unique_lock lock(m_mval->mtx);
            return safe_ptr_proxy<T>(std::move(lock), m_mval->val.get());
        }

        const safe_ptr_proxy<T> operator->() const
        {
            std::unique_lock lock(m_mval->mtx);
            return safe_ptr_proxy<T>(std::move(lock), m_mval->val.get());
        }

    private:
        struct mutexed_val
        {
            mutable std::recursive_mutex mtx;
            std::unique_ptr<T> val;
        };
        std::shared_ptr<mutexed_val> m_mval;
    };

    template<typename T, typename...Args>
    safe_ptr<T> make_safe(Args&&...args)
    {
        auto val = new T(std::forward<Args>(args)...);
        try {
            return safe_ptr<T>(val);
        } catch(...) {
            delete val;
            throw;
        }
    }
}
