#include <common-lib/mpl/type-traits.h>

using namespace vshalygin::cl;

static_assert(std::is_same_v<tuple_element_t<std::tuple<const int &>, 0>,
                             const int &>);
static_assert(std::is_same_v<tuple_element_t<std::tuple<const int &> &, 0>,
                             const int &>);
static_assert(std::is_same_v<tuple_element_t<std::tuple<const int &> &&, 0>,
                             const int &>);
static_assert(std::is_same_v<tuple_element_t<const std::tuple<const int &>, 0>,
                             const int &>);
static_assert(std::is_same_v<tuple_element_t<const std::tuple<const int &> &, 0>,
                             const int &>);
static_assert(std::is_same_v<tuple_element_t<const std::tuple<const int &> &&, 0>,
                             const int &>);
static_assert(std::is_same_v<tuple_element_t<volatile std::tuple<const int &>, 0>,
                             const int &>);
static_assert(std::is_same_v<tuple_element_t<volatile std::tuple<const int &> &, 0>,
                             const int &>);
static_assert(std::is_same_v<tuple_element_t<volatile std::tuple<const int &> &&, 0>,
                             const int &>);
static_assert(std::is_same_v<tuple_element_t<const volatile std::tuple<const int &>, 0>,
                             const int &>);
static_assert(std::is_same_v<tuple_element_t<const volatile std::tuple<const int &> &, 0>,
                             const int &>);
static_assert(std::is_same_v<tuple_element_t<const volatile std::tuple<const int &> &&, 0>,
                             const int &>);

static_assert(std::is_same_v<tuple_element_t<std::tuple<int, double>, 1>, double>);
static_assert(std::is_same_v<tuple_element_t<std::tuple<int, double&>, 1>, double &>);
static_assert(std::is_same_v<tuple_element_t<std::tuple<int, const double &&>, 1>,
                             const double &&>);
