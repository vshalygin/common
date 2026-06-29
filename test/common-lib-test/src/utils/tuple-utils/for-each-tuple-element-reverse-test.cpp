#include <common-lib/utils/tuple-utils.h>
#include <gtest/gtest.h>
#include <string>

using namespace vshalygin::cl;
using namespace testing;

TEST(ForEachTupleElementReverse, BasicTest)
{
    std::stringstream ss;
    auto f = [&ss](const auto &v) {
        ss << v << " ";
    };
    std::tuple<int, char, size_t> t{ 7, 'l', 56 };

    for_each_tuple_element_reverse(t, f);

    ASSERT_EQ(ss.str(), "56 l 7 ");
}

TEST(ForEachTupleElementReverse, DoNotCopyObjects)
{
    std::stringstream ss;
    auto f = [&ss](const auto &v) {
        ss << *v << " ";
    };
    std::tuple t{ std::make_unique<int>(2), std::make_unique<int>(6) };

    for_each_tuple_element_reverse(t, f);

    EXPECT_EQ(ss.str(), "6 2 ");
    EXPECT_TRUE(std::get<0>(t));
    EXPECT_TRUE(std::get<1>(t));
}

TEST(ForEachTupleElementReverse, MovesObjects)
{
    std::stringstream ss;
    auto f = [&ss](auto &&v) {
        std::unique_ptr<int> temp = std::move(v);
        ss << *temp << " ";
    };
    std::tuple t{ std::make_unique<int>(2), std::make_unique<int>(6) };

    for_each_tuple_element_reverse(std::move(t), f);

    EXPECT_EQ(ss.str(), "6 2 ");
    EXPECT_FALSE(std::get<0>(t));
    EXPECT_FALSE(std::get<1>(t));
}
