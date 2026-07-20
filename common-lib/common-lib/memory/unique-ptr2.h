#pragma once
#include <utility>
#include <type_traits>

namespace vshalygin::cl {
    template<typename Allocator, typename T>
    class unique_ptr2;

    template<typename Allocator, typename T, typename...Args>
    unique_ptr2<Allocator, T> make_unique2(Args&&...args);

    template<typename Allocator, typename T>
    class unique_ptr2 final
    {
        static_assert(!std::is_void_v<T>, "unique_ptr2 cannot be used with void");

        static_assert(std::is_empty_v<Allocator> &&
                      std::is_trivially_copyable_v<Allocator> &&
                      std::is_trivially_default_constructible_v<Allocator>,
                      "Allocator does not match requirements");

        template<typename Allocator, typename T, typename...Args>
        friend unique_ptr2<Allocator, T> make_unique2(Args&&...args);

        template<typename Allocator, typename T>
        friend class unique_ptr2;

    public:
        unique_ptr2() noexcept = default;

        template<typename Y, typename = std::enable_if_t<std::is_convertible_v<Y *, T *>>>
        unique_ptr2(unique_ptr2<Allocator, Y> &&other) noexcept
            : m_alloc_memory(other.m_alloc_memory)
            , m_object(other.m_object)
        {
            other.m_alloc_memory = nullptr;
            other.m_object = nullptr;
        }

        ~unique_ptr2()
        {
            reset();
        }

        unique_ptr2(const unique_ptr2 &) = delete;
        unique_ptr2 &operator=(const unique_ptr2 &) = delete;

        unique_ptr2(unique_ptr2 &&other) noexcept
            : m_alloc_memory(other.m_alloc_memory)
            , m_object(other.m_object)
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

                other.m_alloc_memory = nullptr;
                other.m_object = nullptr;
            }
            return *this;
        }

        template<typename Y, typename = std::enable_if_t<std::is_convertible_v<Y *, T *>>>
        unique_ptr2 &operator=(unique_ptr2<Allocator, Y> &&other) noexcept
        {
            reset();

            m_alloc_memory = other.m_alloc_memory;
            m_object = other.m_object;

            other.m_alloc_memory = nullptr;
            other.m_object = nullptr;

            return *this;
        }

        void reset() noexcept
        {
            if(m_alloc_memory) {
                m_object->~T();
                Allocator{}.deallocate(m_alloc_memory);
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
        void *m_alloc_memory = nullptr;
        T *m_object = nullptr;
    };

    template< typename Allocator, typename T, typename...Args>
    unique_ptr2<Allocator, T> make_unique2(Args&&...args)
    {
        T *ptr = Allocator{}.template allocate<T>();
        try {
            new (ptr) T(std::forward<Args>(args)...);
        } catch (...) {
            Allocator{}.deallocate(static_cast<void *>(ptr));
            throw;
        }

        unique_ptr2<Allocator, T> ans;
        ans.m_alloc_memory = static_cast<void *>(ptr);
        ans.m_object = ptr;

        return ans;
    }
}
