#include <common-lib/utils/tuple-utils.h>
#include <gtest/gtest.h>
#include <memory>

using namespace vshalygin::cl;
using namespace testing;

TEST(SwapFirstTwoTupleElements, BasicCheck)
{
    static float f = 34;
    std::tuple<int, double, std::string, float &> t{ 4, 8.0, "test", f};

    auto r = swap_first_two_tuple_elements(t);
    static_assert(std::is_same_v<decltype(r),
                                 std::tuple<double, int, std::string, float &>>);

    EXPECT_EQ(std::get<0>(r), 8.0);
    EXPECT_EQ(std::get<1>(r), 4);
    EXPECT_EQ(std::get<2>(r), "test");
    EXPECT_EQ(std::get<3>(r), 34);
}

TEST(SwapFirstTwoTupleElements, MoveObjects)
{
    static float f = 34;
    auto t = std::make_tuple(std::make_unique<int>(2), 4, std::make_unique<double>(8.0));

    auto r = swap_first_two_tuple_elements(std::move(t));
    static_assert(std::is_same_v<decltype(r),
                                std::tuple<int, std::unique_ptr<int>, std::unique_ptr<double>>>) ;

    EXPECT_EQ(std::get<0>(r), 4);
    EXPECT_EQ(*std::get<1>(r), 2);
    EXPECT_EQ(*std::get<2>(r), 8.0);
}
