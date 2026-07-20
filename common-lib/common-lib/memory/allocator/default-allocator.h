#pragma once
#include <cstdlib>
#include <new>

namespace vshalygin::cl {
    class default_allocator
    {
    public:
        template<typename T>
        T *allocate() const
        {
            T *ans;
#ifdef _MSC_VER
            ans = static_cast<T *>(_aligned_malloc(sizeof(T), alignof(T)));
#else
            constexpr std::size_t alignment = alignof(T);
            constexpr std::size_t size =
                (sizeof(T) + alignment - 1) / alignment * alignment;

            ans = static_cast<T *>(std::aligned_alloc(alignment, size));
#endif

            if(!ans) {
                throw std::bad_alloc{};
            }

            return ans;
        }

        void deallocate(void *ptr) const noexcept
        {
#ifdef _MSC_VER
            _aligned_free(ptr);
#else
            std::free(ptr);
#endif       
        }
    };
}