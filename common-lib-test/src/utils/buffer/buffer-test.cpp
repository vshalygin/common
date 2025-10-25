#include <common-lib/utils/buffer/buffer.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace vsh::common_lib;
using namespace testing;

TEST(Buffer, ReturnsNullptrAfterDefaultConstruction)
{
    buffer sut;

    ASSERT_THAT(sut.data(), IsNull());
}

TEST(Buffer, ReturnsZeroSizeAfterDefaultConstruction)
{
    buffer sut;

    ASSERT_THAT(sut.size(), Eq(0));
}

TEST(Buffer, ReturnsNonZeroBufferAfterConstructionWithParameter)
{
    buffer sut(3);

    ASSERT_THAT(sut.data(), NotNull());
}

TEST(Buffer, ReturnsSizeAfterConstructionWithSizeParameter)
{
    buffer sut(3);

    ASSERT_THAT(sut.size(), Eq(3));
}

TEST(Buffer, MovesToAnotherObjectByCopying)
{
    buffer buf1(3);
    auto data_ptr = buf1.data();

    buffer buf2(std::move(buf1));

    ASSERT_EQ(buf2.data(), data_ptr);
    ASSERT_EQ(buf2.size(), 3);
}

TEST(Buffer, MovesToAnotherObjectByAssignment)
{
    buffer buf1(3);
    auto data_ptr = buf1.data();

    buffer buf2;
    buf2 = std::move(buf1);

    ASSERT_EQ(buf2.data(), data_ptr);
    ASSERT_EQ(buf2.size(), 3);
}

TEST(Buffer, AllocatesNewBufferIfNewSizeIsBiggerThanCurrentOne)
{
    buffer buf(3);
    auto data_ptr = buf.data();

    buf.reallocate(4);

    ASSERT_NE(buf.data(), data_ptr);
    ASSERT_EQ(buf.size(), 4);
}
