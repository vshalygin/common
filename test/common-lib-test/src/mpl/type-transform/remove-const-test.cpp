#include <common-lib/mpl/type-transform.h>

using namespace vshalygin::cl;

static_assert(std::is_same_v<int, remove_const_t<int>>);
static_assert(std::is_same_v<int &, remove_const_t<int &>>);
static_assert(std::is_same_v<int &&, remove_const_t<int &&>>);
static_assert(std::is_same_v<int, remove_const_t<const int>>);
static_assert(std::is_same_v<int &, remove_const_t<const int &>>);
static_assert(std::is_same_v<int &&, remove_const_t<const int &&>>);
static_assert(std::is_same_v<volatile int, remove_const_t<volatile int>>);
static_assert(std::is_same_v<volatile int &, remove_const_t<volatile int &>>);
static_assert(std::is_same_v<volatile int &&, remove_const_t<volatile int &&>>);
static_assert(std::is_same_v<volatile int, remove_const_t<const volatile int>>);
static_assert(std::is_same_v<volatile int &, remove_const_t<const volatile int &>>);
static_assert(std::is_same_v<volatile int &&, remove_const_t<const volatile int &&>>);