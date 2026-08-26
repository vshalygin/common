#include <common-lib/utils/value-proxy.h>
#include <gtest/gtest.h>

using namespace vshalygin::cl;
using namespace testing;

TEST(ValueProxy, InitSelfStore)
{
    int i = 6;
    value_proxy<int &> sut(i, value_proxy_owned);

    sut.to_underlying() = 7;

    ASSERT_EQ(static_cast<int &>(sut), 7);
    ASSERT_EQ(i, 6);
    ASSERT_NE(&static_cast<int &>(sut), &i);
}

TEST(ValueProxy, InitExternalStore)
{
    int i = 8;
    value_proxy<int &> sut(i, value_proxy_external);

    sut.to_underlying() = 7;

    ASSERT_EQ(i, 7);
    ASSERT_EQ(static_cast<int &>(sut), 7);
    ASSERT_EQ(&static_cast<int &>(sut), &i);
}

TEST(ValueProxy, TestConstGetters)
{
    int i = 8;
    const value_proxy<int &> sut1(i, value_proxy_external);

    i = 9;
    ASSERT_EQ(sut1.to_underlying(), 9);
}

TEST(ValueProxy, DestroyStoringValue)
{
    value_proxy<std::unique_ptr<int> &> v(std::make_unique<int>(6), value_proxy_owned);
    (void)v;
}

TEST(ValueProxy, TestRRefGetters)
{
    int i = 8;
    value_proxy<int &&> sut1(std::move(i), value_proxy_external);
    EXPECT_EQ(sut1.to_underlying(), i);

    int ii = 9;
    value_proxy<int &&> sut2(ii, value_proxy_owned);
    EXPECT_NE(sut1.to_underlying(), ii);
}

TEST(ValueProxy, IsNotValidByDefault)
{
    value_proxy<int &&> sut;

    ASSERT_FALSE(sut.is_valid());
}

TEST(ValueProxy, ValidCreatedOwned)
{
    value_proxy<int &&> sut(7, value_proxy_owned);

    ASSERT_TRUE(sut.is_valid());
}

TEST(ValueProxy, ValidCreatedExternal)
{
    int i = 0;
    value_proxy<int &&> sut(std::move(i), value_proxy_external);

    ASSERT_TRUE(sut.is_valid());
}

TEST(ValueProxy, CopyOwned)
{
    value_proxy<int &> sut1(7, value_proxy_owned);
    auto sut2(sut1);

    ASSERT_TRUE(sut1.is_valid());
    ASSERT_TRUE(sut2.is_valid());
    ASSERT_EQ(&sut1.to_underlying(), &sut2.to_underlying());
}

TEST(ValueProxy, CopyExternal)
{
    int i = 9;
    value_proxy<int &> sut1(i, value_proxy_external);
    auto sut2(sut1);

    ASSERT_TRUE(sut1.is_valid());
    ASSERT_TRUE(sut2.is_valid());
    ASSERT_EQ(&sut1.to_underlying(), &sut2.to_underlying());
    ASSERT_EQ(&sut1.to_underlying(), &i);
}

TEST(ValueProxy, CopyEmpty)
{
    value_proxy<int &> sut3;
    auto sut4(sut3);

    ASSERT_FALSE(sut3.is_valid());
    ASSERT_FALSE(sut4.is_valid());
}

TEST(ValueProxy, CopyAssignOwned)
{
    value_proxy<int &> sut1(7, value_proxy_owned);
    value_proxy<int &> sut2(7, value_proxy_owned);
    sut2 = sut1;

    ASSERT_TRUE(sut1.is_valid());
    ASSERT_TRUE(sut2.is_valid());
    ASSERT_EQ(&sut1.to_underlying(), &sut2.to_underlying());
}

TEST(ValueProxy, CopyAssignExternal)
{
    int i = 9;
    value_proxy<int &> sut1(i, value_proxy_external);
    value_proxy<int &> sut2(7, value_proxy_owned);
    sut2 = sut1;

    ASSERT_TRUE(sut1.is_valid());
    ASSERT_TRUE(sut2.is_valid());
    ASSERT_EQ(&sut1.to_underlying(), &sut2.to_underlying());
    ASSERT_EQ(&sut1.to_underlying(), &i);
}

TEST(ValueProxy, CopyAssignEmpty)
{
    value_proxy<int &> sut1;
    value_proxy<int &> sut2;
    sut2 = sut1;

    ASSERT_FALSE(sut1.is_valid());
    ASSERT_FALSE(sut2.is_valid());
}

TEST(ValueProxy, MoveOwned)
{
    value_proxy<int &> sut1(7, value_proxy_owned);
    auto sut2(std::move(sut1));

    ASSERT_FALSE(sut1.is_valid());
    ASSERT_TRUE(sut2.is_valid());
}

TEST(ValueProxy, MoveExternal)
{
    int i = 9;
    value_proxy<int &> sut1(i, value_proxy_external);
    auto sut2(std::move(sut1));

    ASSERT_FALSE(sut1.is_valid());
    ASSERT_TRUE(sut2.is_valid());
    ASSERT_EQ(&sut2.to_underlying(), &i);
}

TEST(ValueProxy, MoveEmpty)
{
    value_proxy<int &> sut3;
    auto sut4(std::move(sut3));

    ASSERT_FALSE(sut3.is_valid());
    ASSERT_FALSE(sut4.is_valid());
}

TEST(ValueProxy, MoveAssignOwned)
{
    value_proxy<int &> sut1(7, value_proxy_owned);
    value_proxy<int &> sut2(7, value_proxy_owned);
    auto &sut1_alias = sut1;
    sut1 = std::move(sut1_alias);
    sut2 = std::move(sut1);

    ASSERT_FALSE(sut1.is_valid());
    ASSERT_TRUE(sut2.is_valid());
}

TEST(ValueProxy, MoveAssignExternal)
{
    int i = 9;
    value_proxy<int &> sut1(i, value_proxy_external);
    value_proxy<int &> sut2(7, value_proxy_owned);
    auto &sut1_alias = sut1;
    sut1 = std::move(sut1_alias);
    sut2 = std::move(sut1);

    ASSERT_FALSE(sut1.is_valid());
    ASSERT_TRUE(sut2.is_valid());
    ASSERT_EQ(&sut2.to_underlying(), &i);
}

TEST(ValueProxy, MoveAssignEmpty)
{
    value_proxy<int &> sut1;
    value_proxy<int &> sut2;
    auto &sut1_alias = sut1;
    sut1 = std::move(sut1_alias);
    sut2 = std::move(sut1);

    ASSERT_FALSE(sut1.is_valid());
    ASSERT_FALSE(sut2.is_valid());
}

TEST(ValueProxy, MayStoreVoidType)
{
    value_proxy<void> sut;

    sut.to_underlying();
    ASSERT_TRUE(sut.is_valid());
}
