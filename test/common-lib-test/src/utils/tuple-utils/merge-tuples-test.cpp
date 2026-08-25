#include <common-lib/utils/tuple-utils.h>
#include <gtest/gtest.h>
#include <string>

using namespace vshalygin::cl;
using namespace testing;

TEST(MergeTuples, BasicTest)
{
    int i = 34;
    std::string s = "7";
    std::tuple<int, const std::string &> t1{ 6, s };
    std::tuple<char, int *> t2{ 'c', &i };

    auto r1 = merge_tuples(t1, t2);
    auto r2 = merge_tuples(t2, t1);
    static_assert(std::is_same_v<decltype(r1), std::tuple<int, const std::string &,
                                                          char, int *>>);
    static_assert(std::is_same_v<decltype(r2), std::tuple<char, int *,
                                                          int, const std::string &>>);

    EXPECT_EQ(std::get<0>(r1), 6);
    EXPECT_EQ(std::get<1>(r1), "7");
    EXPECT_EQ(std::get<2>(r1), 'c');
    EXPECT_EQ(*std::get<3>(r1), 34);
    EXPECT_EQ(std::get<0>(r2), 'c');
    EXPECT_EQ(*std::get<1>(r2), 34);
    EXPECT_EQ(std::get<2>(r2), 6);
    EXPECT_EQ(std::get<3>(r2), "7");

    EXPECT_EQ(std::get<3>(r1), &i);
    EXPECT_EQ(std::get<1>(r2), &i);

    EXPECT_EQ(std::get<1>(r1).data(), s.data());
    EXPECT_EQ(std::get<3>(r2).data(), s.data());
}

TEST(MergeTuples, MoveObjects)
{
    std::tuple<int, std::unique_ptr<int>> t1{ 6, std::make_unique<int>(34)};
    std::tuple<int, std::unique_ptr<int>> t2{ 5, std::make_unique<int>(60) };

    auto r = merge_tuples(std::move(t1), std::move(t2));

    EXPECT_EQ(std::get<0>(r), 6);
    EXPECT_EQ(*std::get<1>(r), 34);
    EXPECT_EQ(std::get<2>(r), 5);
    EXPECT_EQ(*std::get<3>(r), 60);
    EXPECT_FALSE(std::get<1>(t1));
    EXPECT_FALSE(std::get<1>(t2));
}

TEST(MergeTuples, MayMergeMoreThanTwoObject)
{
    char ch = 'a';
    std::tuple<std::string> t1{ "6" };
    std::tuple<double> t2{ 5 };
    std::tuple<char &> t3{ ch };

    auto r = merge_tuples(t1, t2, t3);
    static_assert(std::is_same_v<decltype(r), std::tuple<std::string, double, char &>>);

    EXPECT_EQ(std::get<0>(r), "6");
    EXPECT_EQ(std::get<1>(r), 5);
    EXPECT_EQ(std::get<2>(r), 'a');
}

TEST(MergeTuples, ValidOnMergingOneTuple)
{
    std::tuple<int> t1{ 6 };

    auto r = merge_tuples(t1);
    static_assert(std::is_same_v<decltype(r), std::tuple<int>>);

    EXPECT_EQ(std::get<0>(r), 6);
}

TEST(MergeTuples, MergesReferenceTypeValues)
{
    int i = 10;
    int ii = 20;
    std::tuple<int &> t1{ i };
    std::tuple<int &> t2{ ii };

    auto r = merge_tuples(std::move(t1), t2);

    i++, ii++;
    EXPECT_EQ(std::get<0>(r), 11);
    EXPECT_EQ(std::get<1>(r), 21);
}

TEST(MergeTuples, MergeEmptyTuples)
{
    std::tuple<> t1{};
    std::tuple<> t2{};

    auto r = merge_tuples(std::move(t1), t2);

    static_assert(std::tuple_size_v<decltype(r)> == 0);
}

TEST(MergeTuples, MergeOneEmptyTuple)
{
    std::tuple<> t1{};
    std::tuple<int> t2{ 6};

    auto r1 = merge_tuples(t1, t2); (void)r1;
    auto r2 = merge_tuples(t2, t1); (void)r2;

    static_assert(std::is_same_v<decltype(r1), std::tuple<int>>);
    static_assert(std::is_same_v<decltype(r2), std::tuple<int>>);
}
