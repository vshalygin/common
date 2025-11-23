#pragma once
#include <utility>
#include <cstdlib>

namespace vsh::common_lib {
    class default_allocator
    {
    public:
        //TODO fix MSVC specific
        template<typename T>
        T *allocate() const
        {
            return static_cast<T *>(_aligned_malloc(alignof(T), sizeof(T)));
        }

        void deallocate(void *ptr) const noexcept
        {
            _aligned_free(ptr);
        }
    };

    template<typename T, typename Allocator = default_allocator>
    class unique_ptr;

    template<typename T, typename Allocator, typename...Args>
    unique_ptr<T, Allocator> make_unique_alloc(const Allocator &allocator, Args&&...args);

    template<typename T, typename Allocator>
    class unique_ptr final
    {
        //TODO add checking for Allocator

        template<typename T, typename Allocator, typename...Args>
        friend unique_ptr<T, Allocator> make_unique_alloc(
            const Allocator &allocator, Args&&...args);

    public:
        explicit unique_ptr(const Allocator &alloc = Allocator()) noexcept
            : ptr_(nullptr)
            , alloc_(alloc)
        {
            static_assert(std::is_nothrow_constructible_v<Allocator, decltype(alloc)>,
                          "allocator is not nothrow copyable");
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
            static_assert(std::is_nothrow_constructible_v<Allocator, decltype(other.alloc_)>,
                          "allocator is not nothrow copyable");
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
            if(ptr_) {
                get_object_ptr()->~T();
                alloc_.deallocate(ptr_);
                ptr_ = nullptr;
            }
        }

        T *get() noexcept
        {
            return get_object_ptr();
        }

        const T *get() const noexcept
        {
            return get_object_ptr();
        }

        T *operator->() noexcept
        {
            return get_object_ptr();
        }

        const T *operator->() const
        {
            return get_object_ptr();
        }

        T &operator*() noexcept
        {
            return *get_object_ptr();
        }

        const T &operator*() const noexcept
        {
            return *get_object_ptr();
        }

        operator bool() const noexcept
        {
            return get_object_ptr();
        }

    private:
        const T *get_object_ptr() const noexcept
        {
            if(!ptr_) {
                return nullptr;
            }

            std::ptrdiff_t offset = *reinterpret_cast<const std::ptrdiff_t *>(ptr_);
            return reinterpret_cast<T *>(static_cast<std::byte *>(ptr_) + offset);
        }

        T *get_object_ptr() noexcept
        {
            return const_cast<T *>(static_cast<const unique_ptr &>(*this).get_object_ptr());
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
