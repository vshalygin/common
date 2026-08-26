#pragma once
#include <common-lib/mpl/type-transform.h>
#include <common-lib/mpl/function-traits.h>
#include <common-lib/utils/type-qualifiers-cast.h>

#include <tuple>
#include <type_traits>

namespace vshalygin::cl::internal {
    template<typename...Args>
    class ftuple
    {
        using tuple = std::tuple<Args...>;

    public:
        template<typename...UArgs,
                 std::enable_if_t<
                     sizeof...(UArgs) != 1 ||
                     (!std::is_same_v<ftuple, remove_type_qualifiers_t<UArgs>> && ...), int> = 0>
        ftuple(UArgs&&...args)
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
    ftuple(Args&&...) -> ftuple<remove_type_qualifiers_t<Args>...>;

    template<typename Func, typename...TupleTypes, size_t...I>
    decltype(auto) apply_impl(Func &&func,
                              ftuple<TupleTypes...> &tuple,
                              std::index_sequence<I...>)
    {
        return func(type_qualifiers_cast<function_arg_t<I, Func>>(std::get<I>(tuple.to_underlying()))...);
    }

    template<typename Func, typename...TupleTypes, size_t...I>
    decltype(auto) apply_impl(Func &&func,
                              ftuple<TupleTypes...> &&tuple,
                              std::index_sequence<I...>)
    {
        return func(type_qualifiers_cast<function_arg_t<I, Func>>(std::get<I>(std::move(tuple.to_underlying())))...);
    }

    template<typename Func, typename...TupleTypes>
    decltype(auto) apply(Func &&func, ftuple<TupleTypes...> &tuple)
    {
        return apply_impl(std::forward<Func>(func),
                          tuple,
                          std::make_index_sequence<sizeof...(TupleTypes)>());
    }

    template<typename Func, typename...TupleTypes>
    decltype(auto) apply(Func &&func, ftuple<TupleTypes...> &&tuple)
    {
        return apply_impl(std::forward<Func>(func),
                          std::move(tuple),
                          std::make_index_sequence<sizeof...(TupleTypes)>());
    }
}
