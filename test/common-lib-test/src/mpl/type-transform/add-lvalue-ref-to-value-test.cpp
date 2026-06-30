#include <common-lib/mpl/type-transform.h>

using namespace vshalygin::cl;

static_assert(std::is_same_v<add_lvalue_ref_to_value_t<int>, int &>);
static_assert(std::is_same_v<add_lvalue_ref_to_value_t<int &>, int &>);
static_assert(std::is_same_v<add_lvalue_ref_to_value_t<int &&>, int &&>);
static_assert(std::is_same_v<add_lvalue_ref_to_value_t<const int>, const int &>);
static_assert(std::is_same_v<add_lvalue_ref_to_value_t<const int &>, const int &>);
static_assert(std::is_same_v<add_lvalue_ref_to_value_t<const int &&>, const int &&>);
static_assert(std::is_same_v<add_lvalue_ref_to_value_t<volatile int>, volatile int &>);
static_assert(std::is_same_v<add_lvalue_ref_to_value_t<volatile int &>, volatile int &>);
static_assert(std::is_same_v<add_lvalue_ref_to_value_t<volatile int &&>, volatile int &&>);
static_assert(std::is_same_v<add_lvalue_ref_to_value_t<const volatile int>, const volatile int &>);
static_assert(std::is_same_v<add_lvalue_ref_to_value_t<const volatile int &>, const volatile int &>);
static_assert(std::is_same_v<add_lvalue_ref_to_value_t<const volatile int &&>, const volatile int &&>);
