#include <common-lib/utils/tuple-utils.h>

using namespace vshalygin::cl;

//forward_tuple_element argument is rvalue
static_assert(std::is_same_v<int&&,
    decltype(forward_tuple_element<0>(std::declval<std::tuple<int>>()))>);
static_assert(std::is_same_v<int &,
    decltype(forward_tuple_element<0>(std::declval<std::tuple<int &>>()))>);
static_assert(std::is_same_v<int &&,
    decltype(forward_tuple_element<0>(std::declval<std::tuple<int &&>>()))>);
static_assert(std::is_same_v<int &&,
    decltype(forward_tuple_element<0>(std::declval<std::tuple<int>>()))>);
static_assert(std::is_same_v<const int &,
    decltype(forward_tuple_element<0>(std::declval<std::tuple<const int &>>()))>);
static_assert(std::is_same_v<const int &&,
    decltype(forward_tuple_element<0>(std::declval<std::tuple<const int &&>>()))>);
static_assert(std::is_same_v<volatile int &&,
    decltype(forward_tuple_element<0>(std::declval<std::tuple<volatile int>>()))>);
static_assert(std::is_same_v<volatile int &,
    decltype(forward_tuple_element<0>(std::declval<std::tuple<volatile int &>>()))>);
static_assert(std::is_same_v<volatile int &&,
    decltype(forward_tuple_element<0>(std::declval<std::tuple<volatile int &&>>()))>);
static_assert(std::is_same_v<volatile int &&,
    decltype(forward_tuple_element<0>(std::declval<std::tuple<volatile int>>()))>);
static_assert(std::is_same_v<const volatile int &,
    decltype(forward_tuple_element<0>(std::declval<std::tuple<const volatile int &>>()))>);
static_assert(std::is_same_v<const volatile int &&,
    decltype(forward_tuple_element<0>(std::declval<std::tuple<const volatile int &&>>()))>);

//forward_tuple_element argument is lvalue
static_assert(std::is_same_v<int &,
    decltype(forward_tuple_element<0>(std::declval<std::tuple<int> &>()))>);
static_assert(std::is_same_v<int &,
    decltype(forward_tuple_element<0>(std::declval<std::tuple<int &> &>()))>);
static_assert(std::is_same_v<int &&,
    decltype(forward_tuple_element<0>(std::declval<std::tuple<int &&> &>()))>);
static_assert(std::is_same_v<const int &,
    decltype(forward_tuple_element<0>(std::declval<std::tuple<const int> &>()))>);
static_assert(std::is_same_v<const int &,
    decltype(forward_tuple_element<0>(std::declval<std::tuple<const int &> &>()))>);
static_assert(std::is_same_v<const int &&,
    decltype(forward_tuple_element<0>(std::declval<std::tuple<const int &&> &>()))>);
static_assert(std::is_same_v<volatile int &,
    decltype(forward_tuple_element<0>(std::declval<std::tuple<volatile int> &>()))>);
static_assert(std::is_same_v<volatile int &,
    decltype(forward_tuple_element<0>(std::declval<std::tuple<volatile int &> &>()))>);
static_assert(std::is_same_v<volatile int &&,
    decltype(forward_tuple_element<0>(std::declval<std::tuple<volatile int &&> &>()))>);
static_assert(std::is_same_v<volatile const int &,
    decltype(forward_tuple_element<0>(std::declval<std::tuple<volatile const int> &>()))>);
static_assert(std::is_same_v<volatile const int &,
    decltype(forward_tuple_element<0>(std::declval<std::tuple<volatile const int &> &>()))>);
static_assert(std::is_same_v<volatile const int &&,
    decltype(forward_tuple_element<0>(std::declval<std::tuple<volatile const int &&> &>()))>);


static_assert(std::is_same_v<double &&,
    decltype(forward_tuple_element<1>(std::declval<std::tuple<int, double>>()))>);
static_assert(std::is_same_v<const double &&,
    decltype(forward_tuple_element<1>(std::declval<std::tuple<int, const double>>()))>);
static_assert(std::is_same_v<const double &,
    decltype(forward_tuple_element<1>(std::declval<std::tuple<int, const double> &>()))>);
