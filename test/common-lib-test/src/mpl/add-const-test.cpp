#include <common-lib/mpl/type-transform.h>

using namespace vshalygin::cl;

static_assert(std::is_same_v<add_const_t<int>, const int>);
static_assert(std::is_same_v<add_const_t<int &>, const int &>);
static_assert(std::is_same_v<add_const_t<int &&>, const int &&>);
static_assert(std::is_same_v<add_const_t<const int>, const int>);
static_assert(std::is_same_v<add_const_t<const int &>, const int &>);
static_assert(std::is_same_v<add_const_t<volatile int>, volatile const int>);
static_assert(std::is_same_v<add_const_t<volatile int &>, const volatile int &>);
static_assert(std::is_same_v<add_const_t<volatile int &&>, const volatile int &&>);
static_assert(std::is_same_v<add_const_t<const volatile int>, const volatile int>);
static_assert(std::is_same_v<add_const_t<const volatile int &>, const volatile int &>);
static_assert(std::is_same_v<add_const_t<const volatile int &&>, const volatile int &&>);
