#include <common-lib/utils/tuple-utils.h>

#include <gtest/gtest.h>

using namespace vshalygin::cl;
using namespace testing;

TEST(AddFirstTupleElement, BasicTest)
{
    int i = 46;
    std::tuple<int, double> t{ 1, 8.0 };

    auto r = add_first_tuple_element(t, i);
    static_assert(std::is_same_v<decltype(r), std::tuple<int, int, double>>);

    EXPECT_EQ(std::get<0>(r), 46);
    EXPECT_EQ(std::get<1>(r), 1);
    EXPECT_EQ(std::get<2>(r), 8.0);
}

TEST(AddFirstTupleElement, AddsValueToEmptyTuple)
{
    int i = 46;
    std::tuple t;

    auto r = add_first_tuple_element(t, std::move(i));
    static_assert(std::is_same_v<decltype(r), std::tuple<int>>);

    EXPECT_EQ(std::get<0>(r), 46);
}

TEST(AddFirstTupleElement, MovesObjects)
{
    std::tuple t{std::make_unique<int>(34)};
    auto v = std::make_unique<int>(56);

    auto r = add_first_tuple_element(std::move(t), std::move(v));
    static_assert(std::is_same_v<decltype(r), std::tuple<std::unique_ptr<int>,
                                                         std::unique_ptr<int>>>);

    EXPECT_EQ(*std::get<0>(r), 56);
    EXPECT_EQ(*std::get<1>(r), 34);
    EXPECT_FALSE(std::get<0>(t));
    EXPECT_FALSE(v);
}
