#include <common-lib/synchronization/internal/ordered-lock/is-there-repeating-order-number.h>

using namespace vshalygin::cl::internal;

namespace {
    template<size_t I>
    struct test
    {
        static constexpr size_t order = I;
    };
}

static_assert(!is_there_repeating_order_number_v<>);
static_assert(!is_there_repeating_order_number_v<test<0>>);
static_assert(!is_there_repeating_order_number_v<test<0>, test<1>>);
static_assert(!is_there_repeating_order_number_v<test<1>, test<0>>);
static_assert(!is_there_repeating_order_number_v<test<1>, test<3>, test<0>>);

static_assert(is_there_repeating_order_number_v<test<1>, test<1>>);
static_assert(is_there_repeating_order_number_v<test<1>, test<0>, test<1>>);
static_assert(is_there_repeating_order_number_v<test<0>, test<1>, test<1>>);
static_assert(is_there_repeating_order_number_v<test<1>, test<1>, test<0>>);
static_assert(is_there_repeating_order_number_v<test<0>, test<1>, test<2>, test<0>>);