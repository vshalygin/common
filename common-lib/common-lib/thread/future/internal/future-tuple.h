#pragma once
#include <common-lib/mpl/type-transform.h>
#include <tuple>

namespace vshalygin::cl::internal {
    template<typename...Args>
    class future_tuple
    {
        using tuple = std::tuple<remove_type_qualifiers_t<Args>...>;

    public:
        future_tuple(Args&&...args)
            : m_tuple(std::forward<Args>(args)...)
        {}

        tuple &to_underlying()
        {
            return m_tuple;
        }

        const tuple &to_underlying() const
        {
            return m_tuple;
        }

    private:
        std::tuple<remove_type_qualifiers_t<Args>...> m_tuple;
    };
}
