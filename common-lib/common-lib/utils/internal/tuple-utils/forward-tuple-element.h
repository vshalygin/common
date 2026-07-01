#pragma once

namespace vshalygin::cl::internal {
    template<size_t I, typename Tuple>
    constexpr decltype(auto) do_forward_tuple_element(Tuple &&tuple) noexcept
    {
        using type = tuple_element_t<Tuple, I>;
        if constexpr(std::is_reference_v<type>) {
            return static_cast<type>(std::get<I>(std::forward<Tuple>(tuple)));
        } else {
            return std::get<I>(std::forward<Tuple>(tuple));
        }
    }
}
