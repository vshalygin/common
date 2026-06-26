#include <common-lib/utils/type-wrapper.h>

#include <gtest/gtest.h>

using namespace vshalygin::cl;
using namespace testing;

//TODO add test на все поддерживаемые варинты

TEST(TypeWrapperValue, MayStoreValue)
{
    type_wrapper sut(1);

    int other = sut;

    ASSERT_EQ(other, 1);
}

TEST(TypeWrapperValue, MayBeCopied)
{
    type_wrapper sut(1);
    auto sut2 = sut;
    static_assert(std::is_same_v<decltype(sut2), decltype(sut)>);

    ASSERT_EQ(sut2, 1);
}

TEST(TypeWrapperValue, MayBeCopyAssigned)
{
    type_wrapper sut(1);
    type_wrapper sut2(2);
    sut2 = sut;

    ASSERT_EQ(sut2, sut);
}

TEST(TypeWrapperValue, MayBeMoved)
{
    type_wrapper sut(std::make_unique<int>(3));
    auto sut2 = std::move(sut);
    static_assert(std::is_same_v<decltype(sut2), decltype(sut)>);

    ASSERT_EQ(*sut2.to_underlying(), 3);
}

TEST(TypeWrapperValue, MayBeMoveAssigned)
{
    type_wrapper sut(std::make_unique<int>(3));
    type_wrapper sut2(std::unique_ptr<int>{});
    sut2 = std::move(sut);

    ASSERT_EQ(*sut2.to_underlying(), 3);
}

TEST(TypeWrapperValue, IsConvertibleToUnderlyingType)
{
    type_wrapper sut(3);
    const type_wrapper sut2(5);

    int i = sut;
    int ii = sut2;
    int i2 = sut.to_underlying();
    int ii2 = sut2.to_underlying();

    ASSERT_EQ(i, 3);
    ASSERT_EQ(ii, 5);
    ASSERT_EQ(i2, 3);
    ASSERT_EQ(ii2, 5);
}

TEST(TypeWrapperValue, MayBeCopyAssignedByValue)
{
    type_wrapper sut(3);
    sut = sut.to_underlying();

    sut = 5;

    ASSERT_EQ(5, sut);
}

TEST(TypeWrapperValue, MayBeMoveAssignedByValue)
{
    type_wrapper sut(std::make_unique<int>(3));
    sut = std::move(sut.to_underlying());

    sut = std::make_unique<int>(5);

    ASSERT_EQ(5, *sut.to_underlying());
}

TEST(TypeWrapperValue, MayStoreValueOfConstType)
{
    type_wrapper<const int> sut(std::move(5));
    
    ASSERT_EQ(sut, 5);
}

TEST(TypeWrapperRef, MayHoldAReference)
{
    int i = 1;

    type_wrapper<int &> sut(i);
    const type_wrapper<int &> sut2(i);

    ASSERT_EQ(1, sut);
    ASSERT_EQ(1, sut.to_underlying());
    ASSERT_EQ(1, sut2);
    ASSERT_EQ(1, sut2.to_underlying());
}

TEST(TypeWrapperRef, MayBeCopied)
{
    int i = 0;
    type_wrapper<int &> sut(i);
    auto sut2 = sut;
    static_assert(std::is_same_v<decltype(sut2), decltype(sut)>);

    ASSERT_EQ(sut2, 0);
}

TEST(TypeWrapperRef, IsConvertibleToUnderlyingType)
{
    int v1 = 3;
    int v2 = 5;
    type_wrapper<int &> sut(v1);
    const type_wrapper<int &> sut2(v2);

    int i = sut;
    int ii = sut2;
    int i2 = sut.to_underlying();
    int ii2 = sut2.to_underlying();

    ASSERT_EQ(i, 3);
    ASSERT_EQ(ii, 5);
    ASSERT_EQ(i2, 3);
    ASSERT_EQ(ii2, 5);
}

TEST(TypeWrapperRef, MayBeCopyAssignedByValue)
{
    int v = 0;
    volatile int v2 = 1;
    type_wrapper<int &> sut(v);
    type_wrapper<volatile int &> sut2(v2);
    sut = sut.to_underlying();
    sut2 = sut2.to_underlying();

    sut = 5;
    sut2 = 10;

    ASSERT_EQ(5, sut);
    ASSERT_EQ(10, sut2);
}

TEST(TypeWrapperRef, MayBeMoveAssignedByValue)
{
    auto v = std::make_unique<int>(2);
    type_wrapper<std::unique_ptr<int> &> sut(v);
    sut = std::move(sut.to_underlying());

    sut = std::make_unique<int>(5);

    ASSERT_EQ(5, *sut.to_underlying());
}

TEST(TypeWrapperRef, RValueReferenceMayBeRetrieved)
{
    static int i = 9;
    static int ii = 10;
    type_wrapper<int &&> sut(std::move(i));
    type_wrapper<const int &&> sut2(std::move(i));
    
    auto &&t = sut.to_underlying();
    auto &&tt = sut2.to_underlying();
    static_assert(std::is_same_v<const int &&, decltype(tt)>);

    t = 10;

    ASSERT_EQ(i, 10);
}
