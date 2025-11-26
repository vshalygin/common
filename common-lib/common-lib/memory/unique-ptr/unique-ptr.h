#pragma once
#include <common-lib/memory/allocator/default-allocator.h>

#include <utility>
#include <type_traits>

namespace vsh::cl {
    template<typename T, typename Allocator = default_allocator>
    class unique_ptr;

    template<typename T, typename Allocator, typename...Args>
    unique_ptr<T, Allocator> make_unique_alloc(const Allocator &allocator, Args&&...args);

    template<typename T, typename Allocator>
    class unique_ptr final
    {
        static_assert(std::is_nothrow_copy_constructible_v<Allocator>,
                      "allocator is not noexcept copyable");

        template<typename T, typename Allocator, typename...Args>
        friend unique_ptr<T, Allocator> make_unique_alloc(
            const Allocator &allocator, Args&&...args);

        template<typename T, typename Allocator>
        friend class unique_ptr;

    public:
        explicit unique_ptr(const Allocator &alloc = Allocator()) noexcept
            : ptr_(nullptr)
            , alloc_(alloc)
        {}

        template<typename Y, typename = std::enable_if_t<std::is_base_of_v<T, Y>>>
        unique_ptr(unique_ptr<Y, Allocator> &&other) noexcept
            : ptr_(other.ptr_)
            , alloc_(other.alloc_)
        {
            if(!ptr_) {
                return;
            }

            Y *other_ptr = other.get();
            std::ptrdiff_t additional_offset =
                reinterpret_cast<std::byte *>(static_cast<T *>(other_ptr)) -
                reinterpret_cast<std::byte *>(other_ptr);

            std::ptrdiff_t &offset = *reinterpret_cast<std::ptrdiff_t *>(ptr_);
            offset += additional_offset;

            other.ptr_ = nullptr;
        }

        ~unique_ptr()
        {
            reset();
        }

        unique_ptr(unique_ptr &) = delete;
        unique_ptr &operator=(unique_ptr &) = delete;

        unique_ptr(unique_ptr &&other) noexcept
            : ptr_(nullptr)
            , alloc_(other.alloc_)
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

        template<typename Y, typename = std::enable_if_t<std::is_base_of_v<T, Y>>>
        unique_ptr& operator=(unique_ptr<Y, Allocator> &&other) noexcept
        {
            *this = unique_ptr(std::move(other));
            return *this;
        }

        void reset() noexcept
        {
            if(ptr_) {
                get()->~T();
                alloc_.deallocate(ptr_);
                ptr_ = nullptr;
            }
        }

        T *get() noexcept
        {
            return const_cast<T *>(static_cast<const unique_ptr &>(*this).get());
        }

        const T *get() const noexcept
        {
            if(!ptr_) {
                return nullptr;
            }

            std::ptrdiff_t offset = *reinterpret_cast<const std::ptrdiff_t *>(ptr_);
            return reinterpret_cast<T *>(static_cast<std::byte *>(ptr_) + offset);
        }

        T *operator->() noexcept
        {
            return get();
        }

        const T *operator->() const
        {
            return get();
        }

        T &operator*() noexcept
        {
            return *get();
        }

        const T &operator*() const noexcept
        {
            return *get();
        }

        operator bool() const noexcept
        {
            return get();
        }

    private:
        void *ptr_;
        Allocator alloc_;
    };

    template<typename T, typename Allocator, typename...Args>
    unique_ptr<T, Allocator> make_unique_alloc(
        const Allocator &allocator, Args&&...args)
    {
        struct unique_ptr_content
        {
            std::ptrdiff_t offset;
            T object;
        };

        unique_ptr_content *ptr = allocator.allocate<unique_ptr_content>();
        ptr->offset =
            reinterpret_cast<std::byte *>(&ptr->object) - reinterpret_cast<std::byte *>(ptr);

        try {
            new (&(ptr->object)) T(std::forward<Args>(args)...);
        } catch (...) {
            allocator.deallocate(static_cast<void *>(ptr));
            throw;
        }

        unique_ptr<T, Allocator> ans{ allocator };
        ans.ptr_ = ptr;

        return ans;
    }

    template<typename T, typename...Args>
    unique_ptr<T> make_unique(Args&&...args)
    {
        return make_unique_alloc<T, default_allocator, Args...>(
            default_allocator{}, std::forward<Args>(args)...);
    }
}
