#include <common-lib/utils/tuple-utils.h>
#include <gtest/gtest.h>

using namespace vshalygin::cl;
using namespace testing;

TEST(ExtractFirstTupleElement, BasicTest)
{
    int i = 7;
    double d = 9;
    std::tuple<int &, double> t1{ i, 8 };
    std::tuple<int , double &> t2{ 56, d };

    auto r1 = extract_first_tuple_element(t1);
    auto r2 = extract_first_tuple_element(t2);
    static_assert(std::is_same_v<decltype(r1), std::pair<int &, std::tuple<double>>>);
    static_assert(std::is_same_v<decltype(r2), std::pair<int, std::tuple<double &>>>);

    EXPECT_EQ(r1.first, 7);
    EXPECT_EQ(std::get<0>(r1.second), 8);
    EXPECT_EQ(r2.first, 56);
    EXPECT_EQ(std::get<0>(r2.second), 9);
}

TEST(ExtractFirstTupleElement, MoveObjects)
{
    std::tuple t{ std::make_unique<int>(9),
                  std::make_unique<int>(13)};

    auto r = extract_first_tuple_element(std::move(t));

    EXPECT_EQ(*r.first, 9);
    EXPECT_EQ(*std::get<0>(r.second), 13);
}

TEST(ExtractFirstTupleElement, WorkOnArgumentWithOneValue)
{
    std::tuple t{ 1 };

    auto r = extract_first_tuple_element(std::move(t));

    EXPECT_EQ(r.first, 1);
    EXPECT_EQ(std::tuple_size_v<decltype(r.second)>, 0);
}

TEST(ExtractFirstTupleElement, ExtractReferenceTypeValues)
{
    int i = 10;
    int ii = 20;
    std::tuple<int &, int &> t{ i, ii };

    auto r = extract_first_tuple_element(std::move(t));
    static_assert(std::is_same_v<decltype(r), std::pair<int &, std::tuple<int &>>>);

    i++; ii++;
    EXPECT_EQ(r.first, 11);
    EXPECT_EQ(std::get<0>(r.second), 21);
}
