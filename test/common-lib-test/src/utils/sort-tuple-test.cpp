#include <common-lib/utils/tuple-utils.h>
#include <gtest/gtest.h>

using namespace vshalygin::cl;
using namespace testing;

namespace {
    struct value_comparator
    {
        template<typename T, typename U>
        static constexpr bool compare()
        {
            return T::value < U::value;
        }
    };

    class test1
    {
    public:
        static constexpr size_t value = 34;
    };

    class test2
    {
    public:
        static constexpr size_t value = 40;
    };

    class test3
    {
    public:
        static constexpr size_t value = 50;
    };
}

TEST(SortTuple, BasicTest)
{
    std::tuple<test1, test2, test3> t{};

    auto s = sort_tuple<value_comparator>(t); s;

    static_assert(std::is_same_v<decltype(s), std::tuple<test1, test2, test3>>);
}

TEST(SortTuple, SortBaseOnSortCriterionAnyTypeType)
{
    std::tuple<test3, test1, test2> t1{};
    const std::tuple<test3, test1, test2> t2{};

    auto s1 = sort_tuple<value_comparator>(t1);
    auto s2 = sort_tuple<value_comparator>(std::move(t1));
    auto s3 = sort_tuple<value_comparator>(std::tuple<test3, test1, test2>{});
    
    auto s4 = sort_tuple<value_comparator>(t2);
    auto s5 = sort_tuple<value_comparator>(std::move(t2));

    static_assert(std::is_same_v<decltype(s1), std::tuple<test1, test2, test3>>);
    static_assert(std::is_same_v<decltype(s2), std::tuple<test1, test2, test3>>);
    static_assert(std::is_same_v<decltype(s3), std::tuple<test1, test2, test3>>);
    static_assert(std::is_same_v<decltype(s4), std::tuple<test1, test2, test3>>);
}
