#pragma once
#include <cstdlib>

namespace vsh::common_lib {
    class default_allocator
    {
    public:
        //TODO fix MSVC specific
        template<typename T>
        T *allocate() const
        {
            return static_cast<T *>(_aligned_malloc(sizeof(T), alignof(T)));
        }

        void deallocate(void *ptr) const noexcept
        {
            _aligned_free(ptr);
        }
    };
}