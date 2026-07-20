#include <common-lib/mpl/type-transform.h>

using namespace vshalygin::cl;

static_assert(std::is_same_v<remove_c_ref_t<int>, int>);
static_assert(std::is_same_v<remove_c_ref_t<int&>, int>);
static_assert(std::is_same_v<remove_c_ref_t<int&&>, int>);
static_assert(std::is_same_v<remove_c_ref_t<const int>, int>);
static_assert(std::is_same_v<remove_c_ref_t<const int &>, int>);
static_assert(std::is_same_v<remove_c_ref_t<const int &&>, int>);
static_assert(std::is_same_v<remove_c_ref_t<volatile int>, volatile int>);
static_assert(std::is_same_v<remove_c_ref_t<volatile int &>, volatile int>);
static_assert(std::is_same_v<remove_c_ref_t<volatile int &&>, volatile int>);
static_assert(std::is_same_v<remove_c_ref_t<const volatile int>, volatile int>);
static_assert(std::is_same_v<remove_c_ref_t<const volatile int &>, volatile int>);
static_assert(std::is_same_v<remove_c_ref_t<const volatile int &&>, volatile int>);
