#pragma once
#include <common-lib/mpl/type-transform.h>
#include <common-lib/mpl/function-traits.h>
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

    template<typename Func, typename...TupleTypes, size_t...I>
    decltype(auto) apply_impl(Func &&func,
                              future_tuple<TupleTypes...> &tuple,
                              std::index_sequence<I...>)
    {
        return func(static_cast<function_arg_t<I, Func>>(std::get<I>(tuple.to_underlying()))...);
    }

    template<typename Func, typename...TupleTypes, size_t...I>
    decltype(auto) apply_impl(Func &&func,
                              future_tuple<TupleTypes...> &&tuple,
                              std::index_sequence<I...>)
    {
        return func(static_cast<function_arg_t<I, Func>>(std::get<I>(std::move(tuple.to_underlying())))...);
    }

    template<typename Func, typename...TupleTypes>
    decltype(auto) apply(Func &&func, future_tuple<TupleTypes...> &tuple)
    {
        return apply_impl(std::forward<Func>(func),
                          tuple,
                          std::make_index_sequence<sizeof...(TupleTypes)>());
    }

    template<typename Func, typename...TupleTypes>
    decltype(auto) apply(Func &&func, future_tuple<TupleTypes...> &&tuple)
    {
        return apply_impl(std::forward<Func>(func),
                          std::move(tuple),
                          std::make_index_sequence<sizeof...(TupleTypes)>());
    }
}
