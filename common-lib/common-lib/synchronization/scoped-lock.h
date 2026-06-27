#pragma once
#include <common-lib/mpl/type-traits.h>

namespace vshalygin::cl {
    template<typename...MutexType>
    class scoped_lock final
    {
        static_assert((is_lockable<MutexType>::value && ...),
                      "all types must have lock() and unlock()");
        static_assert((!std::is_reference_v<MutexType> && ...),
                      "reference types are not allowed");
        static_assert((!std::is_volatile_v<MutexType> && ...),
                      "volatile types are not allowed");
        static_assert((!std::is_const_v<MutexType> && ...),
                      "const types are not allowed");

    public:
        explicit scoped_lock(MutexType&...mtxs);

        scoped_lock(const scoped_lock &) = delete;
        scoped_lock &operator(const scoped_lock &) = delete;



    private:
    };
}
