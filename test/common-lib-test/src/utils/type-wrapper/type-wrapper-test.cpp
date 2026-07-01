#include <common-lib/utils/type-wrapper.h>

#include <gtest/gtest.h>

using namespace vshalygin::cl;
using namespace testing;

static_assert(std::is_same_v<typename type_wrapper<void>::type, void>);
static_assert(std::is_same_v<typename type_wrapper<int>::type, int>);
static_assert(std::is_same_v<typename type_wrapper<int&>::type, int&>);
static_assert(std::is_same_v<typename type_wrapper<int&&>::type, int&&>);
static_assert(std::is_same_v<typename type_wrapper<const int>::type, const int>);
static_assert(std::is_same_v<typename type_wrapper<const int &>::type, const int &>);
static_assert(std::is_same_v<typename type_wrapper<const int &&>::type, const int &&>);
static_assert(std::is_same_v<typename type_wrapper<volatile int>::type, volatile int>);
static_assert(std::is_same_v<typename type_wrapper<volatile int &>::type, volatile int &>);
static_assert(std::is_same_v<typename type_wrapper<volatile int &&>::type, volatile int &&>);
static_assert(std::is_same_v<typename type_wrapper<const volatile int>::type, const volatile int>);
static_assert(std::is_same_v<typename type_wrapper<const volatile int &>::type, const volatile int &>);
static_assert(std::is_same_v<typename type_wrapper<const volatile int &&>::type, const volatile int &&>);

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
    auto &&i2 = sut.to_underlying();
    auto &&ii2 = sut2.to_underlying();
    static_assert(std::is_same_v<decltype(i2), int &>);
    static_assert(std::is_same_v<decltype(ii2), const int &>);

    ASSERT_EQ(i, 3);
    ASSERT_EQ(ii, 5);
    ASSERT_EQ(i2, 3);
    ASSERT_EQ(ii2, 5);
}

TEST(TypeWrapperValue, MayBeCopyAssignedByValue)
{
    int i = 5;
    const int i2 = 7;
    volatile int i3 = 9;
    const volatile int i4 = 12;

    type_wrapper sut(3);
    sut = sut.to_underlying();

    sut = i;
    sut = i2;
    sut = i3;
    sut = i4;
    sut = std::move(i);
    sut = std::move(i2);
    sut = std::move(i3);
    sut = std::move(i4);

    ASSERT_EQ(12, sut);
}

TEST(TypeWrapperValue, MayBeMoveAssignedByValue)
{
    type_wrapper sut(std::make_unique<int>(3));
    sut = std::move(sut.to_underlying());

    sut = std::make_unique<int>(5);

    ASSERT_EQ(5, *sut.to_underlying());
}

TEST(TypeWrapperValue, MayStoreAllNonReferenceTypes)
{
    type_wrapper<int> s1(5);
    type_wrapper<const int> s2(6);
    type_wrapper<volatile int> s3(7);
    type_wrapper<const volatile int> s4(8);
    
    ASSERT_EQ(s1, 5);
    ASSERT_EQ(s2, 6);
    ASSERT_EQ(s3, 7);
    ASSERT_EQ(s4, 8);
}

TEST(TypeWrapperValue, NonConstHoldingTypeMayBeAssignable)
{
    type_wrapper<int> s1(5);
    type_wrapper<volatile int> s3(7);

    s1 = 10;
    s3 = 7;

    ASSERT_EQ(s1, 10);
    ASSERT_EQ(s3, 7);
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
    static_assert(std::is_same_v<int &&, decltype(t)>);
    static_assert(std::is_same_v<const int &&, decltype(tt)>);

    t = 10;

    ASSERT_EQ(i, 10);
}

TEST(TypeWrapperRef, MayHoldAnyTypeOfReference)
{
    static int i = 9;
    static volatile int ii = 10;
    static const int iii = 9;
    static const volatile int iiii = 10;


    type_wrapper<int &> s1(i);
    type_wrapper<int &&> s2(std::move(i));
    type_wrapper<const int &> s3(i);
    type_wrapper<const int &&> s4(std::move(i));
    type_wrapper<volatile int &> s5(ii);
    type_wrapper<volatile int &&> s6(std::move(ii));
    type_wrapper<const volatile int &> s7(ii);
    type_wrapper<const volatile int &&> s8(std::move(ii));

    const type_wrapper<int &> s9(i);
    const type_wrapper<int &&> s10(std::move(i));
    const type_wrapper<const int &> s11(i);
    const type_wrapper<const int &&> s12(std::move(i));
    const type_wrapper<volatile int &> s13(ii);
    const type_wrapper<volatile int &&> s14(std::move(ii));
    const type_wrapper<const volatile int &> s15(ii);
    const type_wrapper<const volatile int &&> s16(std::move(ii));

    auto &&v1 = s1.to_underlying();
    auto &&v2 = s2.to_underlying();
    auto &&v3 = s3.to_underlying();
    auto &&v4 = s4.to_underlying();
    auto &&v5 = s5.to_underlying();
    auto &&v6 = s6.to_underlying();
    auto &&v7 = s7.to_underlying();
    auto &&v8 = s8.to_underlying();
    auto &&v9 = s9.to_underlying();
    auto &&v10 = s10.to_underlying();
    auto &&v11 = s11.to_underlying();
    auto &&v12 = s12.to_underlying();
    auto &&v13 = s13.to_underlying();
    auto &&v14 = s14.to_underlying();
    auto &&v15 = s15.to_underlying();
    auto &&v16 = s16.to_underlying();

    static_assert(std::is_same_v<decltype(v1), int &>);
    static_assert(std::is_same_v<decltype(v2), int &&>);
    static_assert(std::is_same_v<decltype(v3), const int &>);
    static_assert(std::is_same_v<decltype(v4), const int &&>);
    static_assert(std::is_same_v<decltype(v5), volatile int &>);
    static_assert(std::is_same_v<decltype(v6), volatile int &&>);
    static_assert(std::is_same_v<decltype(v7), const volatile int &>);
    static_assert(std::is_same_v<decltype(v8), const volatile int &&>);
    static_assert(std::is_same_v<decltype(v9), const int &>);
    static_assert(std::is_same_v<decltype(v10), const int &&>);
    static_assert(std::is_same_v<decltype(v11), const int &>);
    static_assert(std::is_same_v<decltype(v12), const int &&>);
    static_assert(std::is_same_v<decltype(v13), volatile const int &>);
    static_assert(std::is_same_v<decltype(v14), volatile const int &&>);
    static_assert(std::is_same_v<decltype(v15), volatile const int &>);
    static_assert(std::is_same_v<decltype(v16), volatile const int &&>);

}

TEST(TypeWrapperRef, MayBeCopyAssignedByValueWithAnyQualifier)
{
    int i2 = 2;
    const int i3 = 3;
    volatile int i4 = 4;
    const volatile int i5 = 4;

    int v = 0;

    type_wrapper<int &> s1(v);
    s1 = i2;
    s1 = std::move(i2);
    s1 = i3;
    s1 = std::move(i3);
    s1 = i4;
    s1 = std::move(i4);
    s1 = i5;
    s1 = std::move(i5);

    type_wrapper<int &&> s2(std::move(v));
    s2 = i2;
    s2 = std::move(i2);
    s2 = i3;
    s2 = std::move(i3);
    s2 = i4;
    s2 = std::move(i4);
    s2 = i5;
    s2 = std::move(i5);

    type_wrapper<volatile int &> s5(v);
    s5 = i2;
    s5 = std::move(i2);
    s5 = i3;
    s5 = std::move(i3);
    s5 = i4;
    s5 = std::move(i4);
    s5 = i5;
    s5 = std::move(i5);

    type_wrapper<volatile int &&> s6(std::move(v));
    s6 = i2;
    s6 = std::move(i2);
    s6 = i3;
    s6 = std::move(i3);
    s6 = i4;
    s6 = std::move(i4);
    s6 = i5;
    s6 = std::move(i5);
}

TEST(TypeWrapperVoid, BasicTest)
{
    type_wrapper<void> v1;
    const type_wrapper<void> v2;

    static_cast<void>(v1);
    static_assert(std::is_same_v<decltype(v1.to_underlying()), void>);
    static_cast<void>(v2);
    static_assert(std::is_same_v<decltype(v2.to_underlying()), void>);
}

TEST(TypeWrapperVoid, IsCopyConstructable)
{
    type_wrapper<void> v;
    type_wrapper<void> v1(v);
    v;

}

TEST(TypeWrapperVoid, IsCopyAssignable)
{
    type_wrapper<void> v;
    type_wrapper<void> v1;
    v = v1;
}

TEST(TypeWrapperVoid, IsMoveConstructable)
{
    type_wrapper<void> v;
    type_wrapper<void> v1(std::move(v));
    v;

}

TEST(TypeWrapperVoid, IsMoveAssignable)
{
    type_wrapper<void> v;
    type_wrapper<void> v1;
    v = std::move(v1);
}
