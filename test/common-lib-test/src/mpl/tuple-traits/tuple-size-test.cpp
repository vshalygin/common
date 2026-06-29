#include <common-lib/mpl/tuple-traits.h>

using namespace vshalygin::cl;

static_assert(tuple_size_v<std::tuple<>> == 0);
static_assert(tuple_size_v<std::tuple<int>> == 1);
static_assert(tuple_size_v<std::tuple<const int &, double>> == 2);

static_assert(tuple_size_v<std::tuple<int>> == 1);
static_assert(tuple_size_v<std::tuple<int> &> == 1);
static_assert(tuple_size_v<std::tuple<int> &&> == 1);
static_assert(tuple_size_v<const std::tuple<int>> == 1);
static_assert(tuple_size_v<const std::tuple<int> &> == 1);
static_assert(tuple_size_v<const std::tuple<int> &&> == 1);
static_assert(tuple_size_v<volatile std::tuple<int>> == 1);
static_assert(tuple_size_v<volatile std::tuple<int> &> == 1);
static_assert(tuple_size_v<volatile std::tuple<int> &&> == 1);
static_assert(tuple_size_v<const volatile std::tuple<int>> == 1);
static_assert(tuple_size_v<const volatile std::tuple<int> &> == 1);
static_assert(tuple_size_v<const volatile std::tuple<int> &&> == 1);