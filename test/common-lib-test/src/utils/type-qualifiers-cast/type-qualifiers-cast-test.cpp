#include <common-lib/utils/type-qualifiers-cast.h>
#include <gtest/gtest.h>

using namespace vshalygin::cl;
using namespace testing;

TEST(TypeQualifiersCast, Tests)
{
    int i;
    decltype(auto) r1 = type_qualifiers_cast<int>(i); (void)r1;
    static_assert(std::is_same_v<decltype(r1), int>);
    decltype(auto) r2 = type_qualifiers_cast<int &>(i); (void)r2;
    static_assert(std::is_same_v<decltype(r2), int &>);
    decltype(auto) r3 = type_qualifiers_cast<int &&>(i); (void)r3;
    static_assert(std::is_same_v<decltype(r3), int &&>);
    decltype(auto) r4 = type_qualifiers_cast<const int>(i); (void)r4;
    static_assert(std::is_same_v<decltype(r4), int>);
    decltype(auto) r5 = type_qualifiers_cast<const int &>(i); (void)r5;
    static_assert(std::is_same_v<decltype(r5), const int &>);
    decltype(auto) r6 = type_qualifiers_cast<const int &&>(i); (void)r6;
    static_assert(std::is_same_v<decltype(r6), const int &&>);
    decltype(auto) r7 = type_qualifiers_cast<volatile int>(i); (void)r7;
    static_assert(std::is_same_v<decltype(r7), int>);
    decltype(auto) r8 = type_qualifiers_cast<volatile int &>(i); (int)r8;
    static_assert(std::is_same_v<decltype(r8), volatile int &>);
    decltype(auto) r9 = type_qualifiers_cast<volatile int &&>(i); (int)r9;
    static_assert(std::is_same_v<decltype(r9), volatile int &&>);
    decltype(auto) r10 = type_qualifiers_cast<const volatile int>(i); (void)r10;
    static_assert(std::is_same_v<decltype(r10), int>);
    decltype(auto) r11 = type_qualifiers_cast<const volatile int &>(i); (int)r11;
    static_assert(std::is_same_v<decltype(r11), const volatile int &>);
    decltype(auto) r12 = type_qualifiers_cast<const volatile int &&>(i); (int)r12;
    static_assert(std::is_same_v<decltype(r12), const volatile int &&>);

    decltype(auto) r13 = type_qualifiers_cast<int>(std::move(i)); (void)r13;
    static_assert(std::is_same_v<decltype(r13), int>);
    decltype(auto) r14 = type_qualifiers_cast<int &>(std::move(i)); (void)r14;
    static_assert(std::is_same_v<decltype(r14), int &>);
    decltype(auto) r15 = type_qualifiers_cast<int &&>(std::move(i)); (void)r15;
    static_assert(std::is_same_v<decltype(r15), int &&>);
    decltype(auto) r16 = type_qualifiers_cast<const int>(std::move(i)); (void)r16;
    static_assert(std::is_same_v<decltype(r16), int>);
    decltype(auto) r17 = type_qualifiers_cast<const int &>(std::move(i)); (void)r17;
    static_assert(std::is_same_v<decltype(r17), const int &>);
    decltype(auto) r18 = type_qualifiers_cast<const int &&>(std::move(i)); (void)r18;
    static_assert(std::is_same_v<decltype(r18), const int &&>);
    decltype(auto) r19 = type_qualifiers_cast<volatile int>(std::move(i)); (void)r19;
    static_assert(std::is_same_v<decltype(r19), int>);
    decltype(auto) r20 = type_qualifiers_cast<volatile int &>(std::move(i)); EXPECT_EQ(&i, &r20);
    static_assert(std::is_same_v<decltype(r20), volatile int &>);
    decltype(auto) r21 = type_qualifiers_cast<volatile int &&>(std::move(i)); EXPECT_EQ(&i, &r21);
    static_assert(std::is_same_v<decltype(r21), volatile int &&>);
    decltype(auto) r22 = type_qualifiers_cast<const volatile int>(std::move(i)); (void)r22;
    static_assert(std::is_same_v<decltype(r22), int>);
    decltype(auto) r23 = type_qualifiers_cast<const volatile int &>(std::move(i)); EXPECT_EQ(&i, &r23);
    static_assert(std::is_same_v<decltype(r23), const volatile int &>);
    decltype(auto) r24 = type_qualifiers_cast<const volatile int &&>(std::move(i)); EXPECT_EQ(&i, &r24);
    static_assert(std::is_same_v<decltype(r24), const volatile int &&>);
}
