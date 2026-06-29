#include <common-lib/mpl/tuple-transform.h>

using namespace vshalygin::cl;

static_assert(std::is_same_v<remove_last_tuple_type_t<std::tuple<int>>,
                             std::tuple<>>);
static_assert(std::is_same_v<remove_last_tuple_type_t<std::tuple<double, const int &>>,
                             std::tuple<double>>);

static_assert(std::is_same_v<remove_last_tuple_type_t<std::tuple<double, int>>,
                             std::tuple<double>>);
static_assert(std::is_same_v<remove_last_tuple_type_t<std::tuple<double, int> &>,
                             std::tuple<double>>);
static_assert(std::is_same_v<remove_last_tuple_type_t<std::tuple<double, int> &&>,
                             std::tuple<double>>);
static_assert(std::is_same_v<remove_last_tuple_type_t<const std::tuple<double, int>>,
                             std::tuple<double>>);
static_assert(std::is_same_v<remove_last_tuple_type_t<const std::tuple<double, int> &>,
                             std::tuple<double>>);
static_assert(std::is_same_v<remove_last_tuple_type_t<const std::tuple<double, int> &&>,
                             std::tuple<double>>);
static_assert(std::is_same_v<remove_last_tuple_type_t<volatile std::tuple<double, int>>,
                             std::tuple<double>>);
static_assert(std::is_same_v<remove_last_tuple_type_t<volatile std::tuple<double, int> &>,
                             std::tuple<double>>);
static_assert(std::is_same_v<remove_last_tuple_type_t<volatile std::tuple<double, int> &&>,
                             std::tuple<double>>);
static_assert(std::is_same_v<remove_last_tuple_type_t<const volatile std::tuple<double, int>>,
                             std::tuple<double>>);
static_assert(std::is_same_v<remove_last_tuple_type_t<const volatile std::tuple<double, int> &>,
                             std::tuple<double>>);
static_assert(std::is_same_v<remove_last_tuple_type_t<const volatile std::tuple<double, int> &&>,
                             std::tuple<double>>);