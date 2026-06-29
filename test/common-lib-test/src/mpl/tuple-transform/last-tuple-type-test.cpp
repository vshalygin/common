#include <common-lib/mpl/tuple-transform.h>

using namespace vshalygin::cl;

static_assert(std::is_same_v<last_tuple_type_t<std::tuple<int>>, int>);
static_assert(std::is_same_v<last_tuple_type_t<std::tuple<double, const int &>>, const int &>);

static_assert(std::is_same_v<last_tuple_type_t<std::tuple<double, int>>, int>);
static_assert(std::is_same_v<last_tuple_type_t<std::tuple<double, int> &>, int>);
static_assert(std::is_same_v<last_tuple_type_t<std::tuple<double, int> &&>, int>);
static_assert(std::is_same_v<last_tuple_type_t<const std::tuple<double, int>>, int>);
static_assert(std::is_same_v<last_tuple_type_t<const std::tuple<double, int> &>, int>);
static_assert(std::is_same_v<last_tuple_type_t<const std::tuple<double, int> &&>, int>);
static_assert(std::is_same_v<last_tuple_type_t<volatile std::tuple<double, int>>, int>);
static_assert(std::is_same_v<last_tuple_type_t<volatile std::tuple<double, int> &>, int>);
static_assert(std::is_same_v<last_tuple_type_t<volatile std::tuple<double, int> &&>, int>);
static_assert(std::is_same_v<last_tuple_type_t<const volatile std::tuple<double, int>>, int>);
static_assert(std::is_same_v<last_tuple_type_t<const volatile std::tuple<double, int> &>, int>);
static_assert(std::is_same_v<last_tuple_type_t<const volatile std::tuple<double, int> &&>, int>);
