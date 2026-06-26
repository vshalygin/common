#include <common-lib/mpl/type-traits.h>
#include <vector>

using namespace vshalygin::cl;

static_assert(!is_std_tuple_v<int>);
static_assert(!is_std_tuple_v<std::vector<int>>);
static_assert(!is_std_tuple_v<int &>);

static_assert(is_std_tuple_v<std::tuple<>>);
static_assert(is_std_tuple_v<std::tuple<int>>);
static_assert(is_std_tuple_v<std::tuple<int &>>);
static_assert(is_std_tuple_v<std::tuple<int &&>>);
static_assert(is_std_tuple_v<std::tuple<const int>>);
static_assert(is_std_tuple_v<std::tuple<const int &>>);
static_assert(is_std_tuple_v<std::tuple<const int &&>>);
static_assert(is_std_tuple_v<std::tuple<volatile int>>);
static_assert(is_std_tuple_v<std::tuple<volatile int &>>);
static_assert(is_std_tuple_v<std::tuple<volatile int &&, double>>);
static_assert(is_std_tuple_v<std::tuple<const volatile int>>);
static_assert(is_std_tuple_v<std::tuple<const volatile int &>>);
static_assert(is_std_tuple_v<std::tuple<const volatile int &&>>);
static_assert(is_std_tuple_v<std::tuple<const volatile int &&, float>>);
static_assert(is_std_tuple_v<std::tuple<const volatile int &&, float &>>);
static_assert(is_std_tuple_v<std::tuple<const volatile int &&, float &&>>);
static_assert(is_std_tuple_v<std::tuple<const volatile int &&, const float>>);
static_assert(is_std_tuple_v<std::tuple<const volatile int &&, const float &>>);
static_assert(is_std_tuple_v<std::tuple<const volatile int &&, const float &&>>);
static_assert(is_std_tuple_v<std::tuple<const volatile int &&, volatile float>>);
static_assert(is_std_tuple_v<std::tuple<const volatile int &&, volatile float &>>);
static_assert(is_std_tuple_v<std::tuple<const volatile int &&, volatile float &&>>);
static_assert(is_std_tuple_v<std::tuple<const volatile int &&, const volatile float>>);
static_assert(is_std_tuple_v<std::tuple<const volatile int &&, const volatile float &>>);
static_assert(is_std_tuple_v<std::tuple<const volatile int &&, const volatile float &&>>);
static_assert(is_std_tuple_v<std::tuple<const volatile int &&, const volatile float &&> &>);
static_assert(is_std_tuple_v<std::tuple<const volatile int &&, const volatile float &&> &&> );
static_assert(is_std_tuple_v<const std::tuple<const volatile int &&,
                                              const volatile float &&>>);
static_assert(is_std_tuple_v<const std::tuple<const volatile int &&,
                                              const volatile float &&> &>);
static_assert(is_std_tuple_v<const std::tuple<const volatile int &&,
                                              const volatile float &&> &&>);
static_assert(is_std_tuple_v<volatile std::tuple<const volatile int &&,
                                                 const volatile float &&>>);
static_assert(is_std_tuple_v<volatile std::tuple<const volatile int &&,
                                              const volatile float &&> &>);
static_assert(is_std_tuple_v<volatile std::tuple<const volatile int &&,
                                                 const volatile float &&> &&>);
static_assert(is_std_tuple_v<const volatile std::tuple<const volatile int &&,
                                                       const volatile float &&>>);
static_assert(is_std_tuple_v<const volatile std::tuple<const volatile int &&,
                                                       const volatile float &&> &>);
static_assert(is_std_tuple_v<const volatile std::tuple<const volatile int &&,
                                                       const volatile float &&> &&>);
