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
            : m_ptr(nullptr)
            , m_alloc(alloc)
        {}

        template<typename Y, typename = std::enable_if_t<std::is_base_of_v<T, Y>>>
        unique_ptr(unique_ptr<Y, Allocator> &&other) noexcept
            : m_ptr(other.m_ptr)
            , m_alloc(other.m_alloc)
        {
            if(!m_ptr) {
                return;
            }

            Y *other_ptr = other.get();
            std::ptrdiff_t additional_offset =
                reinterpret_cast<std::byte *>(static_cast<T *>(other_ptr)) -
                reinterpret_cast<std::byte *>(other_ptr);

            std::ptrdiff_t &offset = *reinterpret_cast<std::ptrdiff_t *>(m_ptr);
            offset += additional_offset;

            other.m_ptr = nullptr;
        }

        ~unique_ptr()
        {
            reset();
        }

        unique_ptr(unique_ptr &) = delete;
        unique_ptr &operator=(unique_ptr &) = delete;

        unique_ptr(unique_ptr &&other) noexcept
            : m_ptr(nullptr)
            , m_alloc(other.m_alloc)
        {
            static_assert(noexcept(std::swap(m_ptr, other.m_ptr)),
                          "std::swap is not noexcept");

            std::swap(m_ptr, other.m_ptr);
        }

        unique_ptr &operator=(unique_ptr &&other) noexcept
        {
            static_assert(noexcept(std::swap(m_ptr, other.m_ptr)),
                          "std::swap is not noexcept");

            std::swap(m_ptr, other.m_ptr);
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
            if(m_ptr) {
                get()->~T();
                m_alloc.deallocate(m_ptr);
                m_ptr = nullptr;
            }
        }

        T *get() noexcept
        {
            return const_cast<T *>(static_cast<const unique_ptr &>(*this).get());
        }

        const T *get() const noexcept
        {
            if(!m_ptr) {
                return nullptr;
            }

            std::ptrdiff_t offset = *reinterpret_cast<const std::ptrdiff_t *>(m_ptr);
            return reinterpret_cast<T *>(static_cast<std::byte *>(m_ptr) + offset);
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
        void *m_ptr;
        Allocator m_alloc;
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
        ans.m_ptr = ptr;

        return ans;
    }

    template<typename T, typename...Args>
    unique_ptr<T> make_unique(Args&&...args)
    {
        return make_unique_alloc<T, default_allocator, Args...>(
            default_allocator{}, std::forward<Args>(args)...);
    }
}
