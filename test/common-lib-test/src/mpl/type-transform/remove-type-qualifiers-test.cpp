#include <common-lib/mpl/type-transform.h>

using namespace vshalygin::cl;

static_assert(std::is_same_v<remove_type_qualifiers_t<int>, int>);
static_assert(std::is_same_v<remove_type_qualifiers_t<int &>, int>);
static_assert(std::is_same_v<remove_type_qualifiers_t<int &&>, int>);
static_assert(std::is_same_v<remove_type_qualifiers_t<const int>, int>);
static_assert(std::is_same_v<remove_type_qualifiers_t<const int &>, int>);
static_assert(std::is_same_v<remove_type_qualifiers_t<const int &&>, int>);
static_assert(std::is_same_v<remove_type_qualifiers_t<volatile int>, int>);
static_assert(std::is_same_v<remove_type_qualifiers_t<volatile int &>, int>);
static_assert(std::is_same_v<remove_type_qualifiers_t<volatile int &&>, int>);
static_assert(std::is_same_v<remove_type_qualifiers_t<const volatile int>, int>);
static_assert(std::is_same_v<remove_type_qualifiers_t<const volatile int &>, int>);
static_assert(std::is_same_v<remove_type_qualifiers_t<const volatile int &&>, int>);
