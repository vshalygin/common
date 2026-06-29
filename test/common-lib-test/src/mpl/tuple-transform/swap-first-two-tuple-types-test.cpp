#include <common-lib/mpl/tuple-transform.h>

using namespace vshalygin::cl;

static_assert(std::is_same_v<
    swap_first_two_tuple_types_t<std::tuple<const int, double &, const char &&>>,
              std::tuple<double &, const int, const char &&>>);
static_assert(std::is_same_v<
    swap_first_two_tuple_types_t<std::tuple<const int, double &, const char &&> &>,
              std::tuple<double &, const int, const char &&>>);
static_assert(std::is_same_v<
    swap_first_two_tuple_types_t<std::tuple<const int, double &, const char &&> &&>,
              std::tuple<double &, const int, const char &&>>);
static_assert(std::is_same_v<
    swap_first_two_tuple_types_t<const std::tuple<const int, double &, const char &&>>,
              std::tuple<double &, const int, const char &&>>);
static_assert(std::is_same_v<
    swap_first_two_tuple_types_t<const std::tuple<const int, double &, const char &&> &>,
              std::tuple<double &, const int, const char &&>>);
static_assert(std::is_same_v<
    swap_first_two_tuple_types_t<const std::tuple<const int, double &, const char &&> &&>,
              std::tuple<double &, const int, const char &&>>);
static_assert(std::is_same_v<
    swap_first_two_tuple_types_t<volatile std::tuple<const int, double &, const char &&>>,
              std::tuple<double &, const int, const char &&>>);
static_assert(std::is_same_v<
    swap_first_two_tuple_types_t<volatile std::tuple<const int, double &, const char &&> &>,
              std::tuple<double &, const int, const char &&>>);
static_assert(std::is_same_v<
    swap_first_two_tuple_types_t<volatile std::tuple<const int, double &, const char &&> &&>,
              std::tuple<double &, const int, const char &&>>);
static_assert(std::is_same_v<
    swap_first_two_tuple_types_t<const volatile std::tuple<const int, double &, const char &&>>,
              std::tuple<double &, const int, const char &&>>);
static_assert(std::is_same_v<
    swap_first_two_tuple_types_t<const volatile std::tuple<const int, double &, const char &&> &>,
              std::tuple<double &, const int, const char &&>>);
static_assert(std::is_same_v<
    swap_first_two_tuple_types_t<volatile std::tuple<const int, double &, const char &&> &&>,
              std::tuple<double &, const int, const char &&>>);

static_assert(std::is_same_v<
    swap_first_two_tuple_types_t<std::tuple<int, double>>,
              std::tuple<double, int>>);
static_assert(std::is_same_v<
    swap_first_two_tuple_types_t<std::tuple<int, double, char, bool, float>>,
              std::tuple<double, int, char, bool, float>>);
