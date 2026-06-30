#include <common-lib/mpl/tuple-transform.h>

using namespace vshalygin::cl;

static_assert(std::is_same_v<
    merge_tuples_t<std::tuple<int>, std::tuple<double>>,
              std::tuple<int, double>>);
static_assert(std::is_same_v<
    merge_tuples_t<std::tuple<int> &, std::tuple<double>>,
              std::tuple<int, double>>);
static_assert(std::is_same_v<
    merge_tuples_t<std::tuple<int> &&, std::tuple<double>>,
              std::tuple<int, double>>);
static_assert(std::is_same_v<
    merge_tuples_t<const std::tuple<int>, std::tuple<double>>,
              std::tuple<int, double>>);
static_assert(std::is_same_v<
    merge_tuples_t<const std::tuple<int> &, std::tuple<double>>,
              std::tuple<int, double>>);
static_assert(std::is_same_v<
    merge_tuples_t<const std::tuple<int> &&, std::tuple<double>>,
              std::tuple<int, double>>);
static_assert(std::is_same_v<
    merge_tuples_t<volatile std::tuple<int>, std::tuple<double>>,
              std::tuple<int, double>>);
static_assert(std::is_same_v<
    merge_tuples_t<volatile std::tuple<int> &, std::tuple<double>>,
              std::tuple<int, double>>);
static_assert(std::is_same_v<
    merge_tuples_t<volatile std::tuple<int> &&, std::tuple<double>>,
              std::tuple<int, double>>);
static_assert(std::is_same_v<
    merge_tuples_t<const volatile std::tuple<int>, std::tuple<double>>,
              std::tuple<int, double>>);
static_assert(std::is_same_v<
    merge_tuples_t<const volatile std::tuple<int> &, std::tuple<double>>,
              std::tuple<int, double>>);
static_assert(std::is_same_v<
    merge_tuples_t<const volatile std::tuple<int> &&, std::tuple<double>>,
              std::tuple<int, double>>);

static_assert(std::is_same_v<
    merge_tuples_t<const std::tuple<int> &&, std::tuple<double>>,
    std::tuple<int, double>>);
static_assert(std::is_same_v<
    merge_tuples_t<const std::tuple<int> &&, std::tuple<double>&>,
              std::tuple<int, double>>);
static_assert(std::is_same_v<
    merge_tuples_t<const std::tuple<int> &&, std::tuple<double>&&>,
              std::tuple<int, double>>);
static_assert(std::is_same_v<
    merge_tuples_t<const std::tuple<int> &&, const std::tuple<double>>,
              std::tuple<int, double>>);
static_assert(std::is_same_v<
    merge_tuples_t<const std::tuple<int> &&, const std::tuple<double> &>,
              std::tuple<int, double>>);
static_assert(std::is_same_v<
    merge_tuples_t<const std::tuple<int> &&, const std::tuple<double> &&>,
              std::tuple<int, double>>);
static_assert(std::is_same_v<
    merge_tuples_t<const std::tuple<int> &&, volatile std::tuple<double>>,
              std::tuple<int, double>>);
static_assert(std::is_same_v<
    merge_tuples_t<const std::tuple<int> &&, volatile std::tuple<double> &>,
              std::tuple<int, double>>);
static_assert(std::is_same_v<
    merge_tuples_t<const std::tuple<int> &&, volatile std::tuple<double> &&>,
              std::tuple<int, double>>);
static_assert(std::is_same_v<
    merge_tuples_t<const std::tuple<int> &&, const volatile std::tuple<double>>,
              std::tuple<int, double>>);
static_assert(std::is_same_v<
    merge_tuples_t<const std::tuple<int> &&, const volatile std::tuple<double> &>,
              std::tuple<int, double>>);
static_assert(std::is_same_v<
    merge_tuples_t<const std::tuple<int> &&, const volatile std::tuple<double> &&>,
              std::tuple<int, double>>);

static_assert(std::is_same_v<merge_tuples_t<std::tuple<>, std::tuple<>>, std::tuple<>>);
static_assert(std::is_same_v<merge_tuples_t<std::tuple<int, double &, const char &&>,
                                            std::tuple<volatile float, bool>>,
                             std::tuple<int, double &, const char &&, volatile float, bool>>);
