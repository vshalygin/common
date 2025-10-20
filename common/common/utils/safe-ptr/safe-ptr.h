#pragma once
#include <memory>
#include <mutex>
#include <utility>

namespace vsh::common {
    template<typename T>
    class safe_ptr_proxy
    {
    public:
        safe_ptr_proxy(std::unique_lock<std::recursive_mutex> lock,
                       T *ptr)
            : lock_(std::move(lock))
            , ptr_(ptr)
        {}

        safe_ptr_proxy(safe_ptr_proxy &) = delete;
        safe_ptr_proxy &operator=(safe_ptr_proxy &) = delete;

        safe_ptr_proxy(safe_ptr_proxy &&) = delete;
        safe_ptr_proxy &operator=(safe_ptr_proxy &&) = delete;

        T *operator->()
        {
            return ptr_;
        }

        const T *operator->() const
        {
            return ptr_;
        }

    private:
        std::unique_lock<std::recursive_mutex> lock_;
        T *ptr_;
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

        safe_ptr(const safe_ptr &) = default;
        safe_ptr& operator=(const safe_ptr &) = default;

        safe_ptr(safe_ptr &&) = default;
        safe_ptr &operator=(safe_ptr &&) = default;

        void reset() noexcept
        {
            mval_.reset();
        }

        template<typename Y>
        void reset(Y *p)
        {
            mval_.reset(new mutexed_val{{}, std::unique_ptr<T>(p)});
        }

        void swap(safe_ptr<T> &other)
        {
            mval_.swap(other.mval_);
        }

        safe_ptr_proxy<T> operator->()
        {
            std::unique_lock lock(mval_->mtx);
            return safe_ptr_proxy<T>(std::move(lock), mval_->val.get());
        }

        const safe_ptr_proxy<T> operator->() const
        {
            std::unique_lock lock(mval_->mtx);
            return safe_ptr_proxy<T>(std::move(lock), mval_->val.get());
        }

    private:
        struct mutexed_val
        {
            mutable std::recursive_mutex mtx;
            std::unique_ptr<T> val;
        };
        std::shared_ptr<mutexed_val> mval_;
    };

    template<typename T, typename...Args>
    safe_ptr<T> make_safe_ptr(Args&&...args)
    {
        auto val = new T(std::forward<Args>(args)...);
        
        return safe_ptr<T>(val);
    }
}
