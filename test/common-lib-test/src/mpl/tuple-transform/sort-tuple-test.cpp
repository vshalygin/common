#include <common-lib/mpl/tuple-transform.h>

using namespace vshalygin::cl;

namespace {
    struct value_comparator
    {
        template<typename T, typename U>
        static constexpr bool compare()
        {
            return T::value < U::value;
        }
    };

    struct value_comparator2
    {
        template<typename T, typename U>
        static constexpr bool compare()
        {
            return T::value > U::value;
        }
    };

    class test1
    {
    public:
        static constexpr size_t value = 34;

        int val = 0;
    };

    class test2
    {
    public:
        static constexpr size_t value = 40;

        int val = 4;
    };

    class test3
    {
    public:
        static constexpr size_t value = 50;

        int val = 5;
    };
}

static_assert(std::is_same_v<sort_tuple_t<std::tuple<test2, test1>, value_comparator>,
                             std::tuple<test1, test2>>);
static_assert(std::is_same_v<sort_tuple_t<std::tuple<test2, test1>, value_comparator2>,
                             std::tuple<test2, test1>>);
static_assert(std::is_same_v<sort_tuple_t<std::tuple<>, value_comparator>,
                             std::tuple<>>);
static_assert(std::is_same_v<sort_tuple_t<std::tuple<test1>, value_comparator>,
                             std::tuple<test1>>);

static_assert(std::is_same_v<sort_tuple_t<std::tuple<test2, test1>, value_comparator>,
                             std::tuple<test1, test2>>);
static_assert(std::is_same_v<sort_tuple_t<std::tuple<test2, test1> &, value_comparator>,
                             std::tuple<test1, test2>>);
static_assert(std::is_same_v<sort_tuple_t<std::tuple<test2, test1> &&, value_comparator>,
                             std::tuple<test1, test2>>);
static_assert(std::is_same_v<sort_tuple_t<const std::tuple<test2, test1>, value_comparator>,
                             std::tuple<test1, test2>>);
static_assert(std::is_same_v<sort_tuple_t<const std::tuple<test2, test1> &, value_comparator>,
                             std::tuple<test1, test2>>);
static_assert(std::is_same_v<sort_tuple_t<const std::tuple<test2, test1> &&, value_comparator>,
                             std::tuple<test1, test2>>);
static_assert(std::is_same_v<sort_tuple_t<volatile std::tuple<test2, test1>, value_comparator>,
                             std::tuple<test1, test2>>);
static_assert(std::is_same_v<sort_tuple_t<volatile std::tuple<test2, test1> &, value_comparator>,
                             std::tuple<test1, test2>>);
static_assert(std::is_same_v<sort_tuple_t<volatile std::tuple<test2, test1> &&, value_comparator>,
                             std::tuple<test1, test2>>);
static_assert(std::is_same_v<sort_tuple_t<const volatile std::tuple<test2, test1>, value_comparator>,
                             std::tuple<test1, test2>>);
static_assert(std::is_same_v<sort_tuple_t<const volatile std::tuple<test2, test1> &, value_comparator>,
                             std::tuple<test1, test2>>);
static_assert(std::is_same_v<sort_tuple_t<const volatile std::tuple<test2, test1> &&, value_comparator>,
                             std::tuple<test1, test2>>);
