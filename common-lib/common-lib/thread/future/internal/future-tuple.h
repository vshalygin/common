#pragma once
#include <common-lib/mpl/type-transform.h>
#include <tuple>

namespace vshalygin::cl::internal {
    template<typename...Args>
    class future_tuple
    {
        using tuple = std::tuple<Args...>;

    public:
        template<typename...UArgs>
        future_tuple(UArgs&&...args)
            : m_tuple(std::forward<UArgs>(args)...)
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
        std::tuple<Args...> m_tuple;
    };

    template<typename...Args>
    future_tuple(Args&&...) -> future_tuple<remove_type_qualifiers_t<Args>...>;
}
