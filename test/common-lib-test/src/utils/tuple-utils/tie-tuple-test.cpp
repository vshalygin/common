#include <common-lib/utils/tuple-utils.h>
#include <gtest/gtest.h>

using namespace vshalygin::cl;

static_assert(std::is_same_v<decltype(tie_tuple(std::declval<std::tuple<int> &>())),
                             std::tuple<int &>>);
static_assert(std::is_same_v<decltype(tie_tuple(std::declval<std::tuple<int &> &>())),
                             std::tuple<int &>>);
static_assert(std::is_same_v<decltype(tie_tuple(std::declval<std::tuple<int &&> &>())),
                             std::tuple<int &&>>);
static_assert(std::is_same_v<decltype(tie_tuple(std::declval<std::tuple<const int> &>())),
                             std::tuple<const int &>>);
static_assert(std::is_same_v<decltype(tie_tuple(std::declval<std::tuple<const int &> &>())),
                             std::tuple<const int &>>);
static_assert(std::is_same_v<decltype(tie_tuple(std::declval<std::tuple<const int &&> &>())),
                             std::tuple<const int &&>>);
static_assert(std::is_same_v<decltype(tie_tuple(std::declval<std::tuple<volatile int> &>())),
                             std::tuple<volatile int &>>);
static_assert(std::is_same_v<decltype(tie_tuple(std::declval<std::tuple<volatile int &> &>())),
                             std::tuple<volatile int &>>);
static_assert(std::is_same_v<decltype(tie_tuple(std::declval<std::tuple<volatile int &&> &>())),
                             std::tuple<volatile int &&>>);
static_assert(std::is_same_v<decltype(tie_tuple(std::declval<std::tuple<const volatile int> &>())),
                             std::tuple<const volatile int &>>);
static_assert(std::is_same_v<decltype(tie_tuple(std::declval<std::tuple<const volatile int &> &>())),
                             std::tuple<const volatile int &>>);
static_assert(std::is_same_v<decltype(tie_tuple(std::declval<std::tuple<const volatile int &&> &>())),
                             std::tuple<const volatile int &&>>);

TEST(TieTuple, BasicTest)
{
    int i = 0;
    int ii = 0;
    std::tuple<int &, int &&,  double> t{ i, std::move(ii), 7 };
    auto tie = tie_tuple(t);
    static_assert(std::is_same_v<decltype(tie), std::tuple<int &, int &&, double &>>);

    std::get<0>(tie) = 1;
    std::get<1>(tie) = 2;
    std::get<2>(tie) = 3;

    EXPECT_EQ(i, 1);
    EXPECT_EQ(ii, 2);
    EXPECT_EQ(std::get<0>(t), 1);
    EXPECT_EQ(std::get<1>(t), 2);
    EXPECT_EQ(std::get<2>(t), 3);
}

TEST(TieTuple, CheckAllTypes)
{
    int i = 0;
    int ii = 0;
    volatile int iii = 0;
    volatile int iiii = 0;
    std::tuple<int,
               int &,
               int &&,
               const int,
               const int &,
               const int &&,
               volatile int,
               volatile int &,
               volatile int &&,
               const volatile int,
               const volatile int &,
               const volatile int &&> t{ 4, i, std::move(ii), 5, i, std::move(ii),
                                         6, iii, std::move(iiii), 7, iii, std::move(iiii), };
    auto tie = tie_tuple(t);
    static_assert(std::is_same_v<decltype(tie), std::tuple<int &,
                                                           int &,
                                                           int &&,
                                                           const int &,
                                                           const int &,
                                                           const int &&,
                                                           volatile int &,
                                                           volatile int &,
                                                           volatile int &&,
                                                           const volatile int &,
                                                           const volatile int &,
                                                           const volatile int &&>>);


    std::get<0>(tie) = 10;
    std::get<1>(tie) = 20;
    std::get<2>(tie) = 30;
    std::get<6>(tie) = 40;
    std::get<7>(tie) = 50;
    std::get<8>(tie) = 60;

    EXPECT_EQ(i, 20);
    EXPECT_EQ(ii, 30);
    EXPECT_EQ(iii, 50);
    EXPECT_EQ(iiii, 60);
    EXPECT_EQ(std::get<0>(t), 10);
    EXPECT_EQ(std::get<1>(t), 20);
    EXPECT_EQ(std::get<2>(t), 30);
    EXPECT_EQ(std::get<3>(t), 5);
    EXPECT_EQ(std::get<4>(t), 20);
    EXPECT_EQ(std::get<5>(t), 30);
    EXPECT_EQ(std::get<6>(t), 40);
    EXPECT_EQ(std::get<7>(t), 50);
    EXPECT_EQ(std::get<8>(t), 60);
    EXPECT_EQ(std::get<9>(t), 7);
    EXPECT_EQ(std::get<10>(t), 50);
    EXPECT_EQ(std::get<11>(t), 60);
}
