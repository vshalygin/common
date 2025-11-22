#pragma once
#include <utility>

namespace vsh::common_lib {
    template<typename T>
    class unique_ptr;

    template<typename T, typename...Args>
    unique_ptr<T> make_unique(Args&&...args);

    template<typename T>
    class unique_ptr
    {
        template<typename T, typename...Args>
        friend unique_ptr<T> make_unique(Args&&...args);

    public:
        explicit unique_ptr() noexcept
            : ptr_(nullptr)
        {}

        ~unique_ptr() noexcept
        {
            reset();
        }

        unique_ptr(unique_ptr &) = delete;
        unique_ptr &operator=(unique_ptr &) = delete;

        unique_ptr(unique_ptr &&other) noexcept
            : unique_ptr()
        {
            static_assert(noexcept(std::swap(ptr_, other.ptr_)),
                          "std::swap is not noexcept");

            std::swap(ptr_, other.ptr_);
        }

        unique_ptr &operator=(unique_ptr &&other) noexcept
        {
            static_assert(noexcept(std::swap(ptr_, other.ptr_)),
                          "std::swap is not noexcept");

            std::swap(ptr_, other.ptr_);

            return *this;
        }

        void reset() noexcept
        {
            delete ptr_;
            ptr_ = nullptr;
        }

        T *get() noexcept
        {
            return ptr_;
        }

        const T *get() const noexcept
        {
            return ptr_;
        }

        T *operator->() noexcept
        {
            return ptr_;
        }

        const T *operator->() const
        {
            return ptr_;
        }

        T &operator*() noexcept
        {
            return *ptr_;
        }

        const T &operator*() const noexcept
        {
            return *ptr_;
        }

        operator bool() const noexcept
        {
            return ptr_;
        }

    private:
        T *ptr_;
    };

    template<typename T, typename...Args>
    unique_ptr<T> make_unique(Args&&...args)
    {
        unique_ptr<T> ans;
        ans.ptr_ = new T(std::forward(args)...);

        return ans;
    }
}
