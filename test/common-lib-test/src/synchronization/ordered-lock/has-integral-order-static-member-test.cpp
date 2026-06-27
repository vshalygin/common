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

static_assert(has_integral_order_static_member_t<intergral_ordered_class>);
static_assert(has_integral_order_static_member_t<const intergral_ordered_class &>);
static_assert(has_integral_order_static_member_t<const intergral_ordered_class &&>);
static_assert(!has_integral_order_static_member_t<non_intergral_ordered_class>);
static_assert(!has_integral_order_static_member_t<const non_intergral_ordered_class&>);
static_assert(!has_integral_order_static_member_t<non_ordered_class>);
static_assert(!has_integral_order_static_member_t<const non_ordered_class &>);
static_assert(!has_integral_order_static_member_t<int>);
static_assert(!has_integral_order_static_member_t<std::string>);