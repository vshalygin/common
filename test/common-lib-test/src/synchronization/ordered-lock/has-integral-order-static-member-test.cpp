#include <common-lib/synchronization/internal/ordered-lock/has-integral-order-static-member.h>
#include <string>

using namespace vshalygin::cl::internal;

namespace {
    struct intergral_ordered_class
    {
        inline static constexpr size_t order = 1;
    };

    struct non_intergral_ordered_class
    {
        inline static constexpr char * order{};
    };

    struct non_ordered_class
    {};
}

static_assert(has_integral_order_static_member_v<intergral_ordered_class>);
static_assert(has_integral_order_static_member_v<const intergral_ordered_class &>);
static_assert(has_integral_order_static_member_v<const intergral_ordered_class &&>);
static_assert(!has_integral_order_static_member_v<non_intergral_ordered_class>);
static_assert(!has_integral_order_static_member_v<const non_intergral_ordered_class&>);
static_assert(!has_integral_order_static_member_v<non_ordered_class>);
static_assert(!has_integral_order_static_member_v<const non_ordered_class &>);
static_assert(!has_integral_order_static_member_v<int>);
static_assert(!has_integral_order_static_member_v<std::string>);