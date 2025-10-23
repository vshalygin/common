#include <rpc-lib/common/buffer/buffer.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace vsh::rpc;
using namespace testing;

TEST(Buffer, ReturnsNullptrAfterDefaultConstruction)
{
    buffer sut;

    ASSERT_THAT(sut.data(), IsNull());
}

TEST(Buffer, ReturnsZeroCapacityAfterDefaultConstruction)
{
    buffer sut;

    ASSERT_THAT(sut.capacity(), Eq(0));
}

TEST(Buffer, ReturnsNonZeroBufferAfterConstructionWithParameter)
{
    buffer sut(3);

    ASSERT_THAT(sut.data(), NotNull());
}

TEST(Buffer, ReturnsCapacityAfterConstructionWithParameter)
{
    buffer sut(3);

    ASSERT_THAT(sut.capacity(), Eq(3));
}

TEST(Buffer, MovesToAnotherObjectByCopying)
{
    buffer buf1(3);
    auto data_ptr = buf1.data();

    buffer buf2(std::move(buf1));

    ASSERT_EQ(buf2.data(), data_ptr);
    ASSERT_EQ(buf2.capacity(), 3);
}

TEST(Buffer, MovesToAnotherObjectByAssignment)
{
    buffer buf1(3);
    auto data_ptr = buf1.data();

    buffer buf2;
    buf2 = std::move(buf1);

    ASSERT_EQ(buf2.data(), data_ptr);
    ASSERT_EQ(buf2.capacity(), 3);
}

TEST(Buffer, DoesNotAllocateNewBufferIfNewCapacityIsEqualToCurrentOne)
{
    buffer buf(3);
    auto data_ptr = buf.data();

    buf.reserve(3);

    ASSERT_EQ(buf.data(), data_ptr);
}

TEST(Buffer, DoesNotAllocateNewBufferIfNewCapacityIsLessThanCurrentOne)
{
    buffer buf(3);
    auto data_ptr = buf.data();

    buf.reserve(2);

    ASSERT_EQ(buf.data(), data_ptr);
}

TEST(Buffer, AllocatesNewBufferIfNewCapacityIsBiggerThanCurrentOne)
{
    buffer buf(3);
    auto data_ptr = buf.data();

    buf.reserve(4);

    ASSERT_NE(buf.data(), data_ptr);
    ASSERT_EQ(buf.capacity(), 4);
}
