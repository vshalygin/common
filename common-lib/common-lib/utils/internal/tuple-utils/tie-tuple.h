#pragma once
#include <common-lib/mpl/type-transform.h>
#include <common-lib/mpl/tuple-traits.h>
#include <tuple>

namespace vshalygin::cl::internal {
    namespace tie_tuple_impl {
        template<typename T>
        decltype(auto) tie_type(T &type)
        {
            if constexpr(std::is_reference_v<T>) {
                return static_cast<T>(type);
            } else {
                return static_cast<T &>(type);
            }
        }

        template<typename...Ts, std::size_t...I>
        auto tie_tuple(std::tuple<Ts...> &t, std::index_sequence<I...>)
        {
            using tuple = std::tuple<Ts...>;
            using ret_t = std::tuple<add_lvalue_ref_to_value_t<Ts>...>;

            return ret_t{ tie_type<tuple_element_t<tuple, I>>(std::get<I>(t))...};
        }
    }

    template<typename...Ts>
    auto do_tie_tuple(std::tuple<Ts...> &t)
    {
        return tie_tuple_impl::tie_tuple(t, std::index_sequence_for<Ts...>{});
    }
}
