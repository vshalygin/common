#pragma once
#include <common-lib/memory/allocator/default-allocator.h>

#include <utility>
#include <type_traits>

namespace vshalygin::cl {
    template<typename T, typename Allocator = default_allocator>
    class unique_ptr2;

    template<typename T, typename Allocator, typename...Args>
    unique_ptr2<T, Allocator> make_unique2(const Allocator &allocator, Args&&...args);

    template<typename T, typename Allocator>
    class unique_ptr2 final
    {
        static_assert(!std::is_void_v<T>, "unique_ptr2 cannot be used with void");

        static_assert(std::is_nothrow_move_constructible_v<Allocator>,
                      "allocator is not noexcept move constructible");
        static_assert(std::is_nothrow_move_assignable_v<Allocator>,
                      "allocator is not noexcept move assignable");

        template<typename T, typename Allocator, typename...Args>
        friend unique_ptr2<T, Allocator> make_unique2(
            std::decay_t<Allocator> &&allocator, Args&&...args);

        template<typename T, typename Allocator>
        friend class unique_ptr2;

    public:
        explicit unique_ptr2(Allocator &&allocator = Allocator()) noexcept
            : m_alloc_memory(nullptr)
            , m_object(nullptr)
            , m_allocator(allocator)
        {}

        template<typename Y, typename = std::enable_if_t<std::is_convertible_v<Y *, T *>>>
        unique_ptr2(unique_ptr2<Y, Allocator> &&other) noexcept
            : m_alloc_memory(other.m_alloc_memory)
            , m_object(other.m_object)
            , m_allocator(std::move(other.m_allocator))
        {
            other.m_alloc_memory = nullptr;
            other.m_object = nullptr;
        }

        ~unique_ptr2()
        {
            reset();
        }

        unique_ptr2(unique_ptr2 &) = delete;
        unique_ptr2 &operator=(unique_ptr2 &) = delete;

        unique_ptr2(unique_ptr2 &&other) noexcept
            : m_alloc_memory(other.m_alloc_memory)
            , m_object(other.m_object)
            , m_allocator(std::move(other.m_allocator))
        {
            other.m_alloc_memory = nullptr;
            other.m_object = nullptr;
        }

        unique_ptr2 &operator=(unique_ptr2 &&other) noexcept
        {
            if(this != &other) {
                reset();

                m_alloc_memory = other.m_alloc_memory;
                m_object = other.m_object;
                m_allocator = std::move(other.m_allocator);

                other.m_alloc_memory = nullptr;
                other.m_object = nullptr;
            }
            return *this;
        }

        template<typename Y, typename = std::enable_if_t<std::is_convertible_v<Y *, T *>>>
        unique_ptr2 &operator=(unique_ptr2<Y, Allocator> &&other) noexcept
        {
            reset();

            m_alloc_memory = other.m_alloc_memory;
            m_object = other.m_object;
            m_allocator = std::move(other.m_allocator);

            other.m_alloc_memory = nullptr;
            other.m_object = nullptr;

            return *this;
        }

        void reset() noexcept
        {
            if(m_alloc_memory) {
                m_object->~T();
                m_allocator.deallocate(m_alloc_memory);
                m_alloc_memory = nullptr;
                m_object = nullptr;
            }
        }

        T *get() noexcept
        {
            return m_object;
        }

        const T *get() const noexcept
        {
            return m_object;
        }

        T *operator->() noexcept
        {
            return get();
        }

        const T *operator->() const noexcept
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
            return get() != nullptr;
        }

    private:
        void *m_alloc_memory;
        T *m_object;

        Allocator m_allocator;
    };

    template<typename T, typename Allocator, typename...Args>
    unique_ptr2<T, Allocator> make_unique2(std::decay_t<Allocator> &&allocator,
                                           Args&&...args)
    {

        T *ptr = allocator.allocate<T>();
        try {
            new (ptr) T(std::forward<Args>(args)...);
        } catch (...) {
            allocator.deallocate(static_cast<void *>(ptr));
            throw;
        }

        unique_ptr2<T, Allocator> ans{ std::move(allocator) };
        ans.m_alloc_memory = static_cast<void *>(ptr);
        ans.m_object = ptr;

        return ans;
    }
}
