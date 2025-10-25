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

TEST(Buffer, GivesAccessToTheElementByIndex)
{
    buffer buf(3);
    *(buf.data()) = 0x1;
    *(buf.data() + 1) = 0x2;
    *(buf.data() + 2) = 0x3;

    ASSERT_EQ(buf[2], 0x3);
}

TEST(Buffer, ConvertsToFalseIfEmpty)
{
    buffer buf;

    ASSERT_FALSE(buf);
}

TEST(Buffer, ConvertsToTrueIfNotEmpty)
{
    buffer buf(3);

    ASSERT_TRUE(buf);
}

TEST(Buffer, GivesAccessToTheElementByIndexWithBoundaryCheck)
{
    buffer buf(3);
    *(buf.data()) = 0x1;
    *(buf.data() + 1) = 0x2;
    *(buf.data() + 2) = 0x3;

    ASSERT_EQ(buf.at(2), 0x3);
}

TEST(Buffer, ThrowsExceptionOnAttemptToGetElementByIndexWithBoundaryCheck)
{
    buffer buf(3);

    ASSERT_THROW(buf.at(10), std::out_of_range);
}

TEST(Buffer, IsAbleToIterateElementsLikeStandartContainer)
{
    buffer buf(3);
    unsigned char byte_val = 0x0;
    *(buf.data()) = byte_val;
    *(buf.data() + 1) = byte_val + 1;
    *(buf.data() + 2) = byte_val + 2;

    for(auto byte : buf) {
        ASSERT_EQ(byte_val++, byte);
    }
}

TEST(BufferIterator, HasIteratorTraits)
{
    bool is_difference_type_same = std::is_same_v<buffer::iterator::difference_type, std::ptrdiff_t>;
    bool is_value_type_type = std::is_same_v<buffer::iterator::value_type, unsigned char>;
    bool is_pointer_type_same = std::is_same_v<buffer::iterator::pointer, unsigned char *>;
    bool is_reference_type_same = std::is_same_v<buffer::iterator::reference, unsigned char &>;
    bool is_iterator_category_same = std::is_same_v<buffer::iterator::iterator_category, std::random_access_iterator_tag>;
    
    ASSERT_TRUE(is_difference_type_same);
    ASSERT_TRUE(is_value_type_type);
    ASSERT_TRUE(is_pointer_type_same);
    ASSERT_TRUE(is_reference_type_same);
    ASSERT_TRUE(is_iterator_category_same);
}

TEST(BufferIterator, IncrementsIteratorWithPrefixIncrementation)
{
    buffer buf(3);
    *(buf.data()) = 0x1;
    *(buf.data() + 1) = 0x2;

    auto iter = buf.begin();
    ASSERT_EQ(*++iter, 0x2);
}

TEST(BufferIterator, IncrementsIteratorWithPostfixIncrementation)
{
    buffer buf(3);
    *(buf.data()) = 0x1;
    *(buf.data() + 1) = 0x2;

    auto iter = buf.begin();
    auto prev_iter = iter++;

    ASSERT_EQ(*prev_iter, 0x1);
    ASSERT_EQ(*iter, 0x2);
}

TEST(BufferIterator, DecrementsIteratorWithPrefixDecrementation)
{
    buffer buf(3);
    *(buf.data()) = 0x1;
    *(buf.data() + 1) = 0x2;

    auto iter = ++buf.begin();
    ASSERT_EQ(*--iter, 0x1);
}

TEST(BufferIterator, DecrementsIteratorWithPostfixDecrementation)
{
    buffer buf(3);
    *(buf.data()) = 0x1;
    *(buf.data() + 1) = 0x2;

    auto iter = ++buf.begin();
    auto prev_iter = iter--;

    ASSERT_EQ(*prev_iter, 0x2);
    ASSERT_EQ(*iter, 0x1);
}

TEST(BufferIterator, AllowsDereferencing)
{
    buffer buf(3);
    *(buf.data()) = 0x1;

    ASSERT_EQ(*buf.begin(), 0x1);
}

TEST(BufferIterator, DetermineEqualIterators)
{
    buffer buf(3);

    ASSERT_TRUE(buf.begin() == buf.begin());
}

TEST(BufferIterator, DetermineNonEqualIterators)
{
    buffer buf(3);

    ASSERT_TRUE(buf.begin() != buf.end());
}

TEST(BufferIterator, SubtractOneIteratorFromAnother)
{
    buffer buf(3);

    ASSERT_EQ(buf.end() - buf.begin(), 3);
}

TEST(BufferIterator, SubtractNumberFromIterator)
{
    buffer buf(3);

    ASSERT_EQ(buf.end() - 3, buf.begin());
}

TEST(BufferIterator, SummarazeAnIteratorWithANumber)
{
    buffer buf(3);

    ASSERT_EQ(buf.begin() + 3, buf.end());
    ASSERT_EQ(3 + buf.begin(), buf.end());
}
