#include <common-lib/utils/buffer.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace vshalygin::cl;
using namespace testing;

TEST(Buffer, CopiesContentToAnotherInstanc)
{
    buffer buf1(2);
    buf1[0] = (std::byte)0x1;
    buf1[1] = (std::byte)0x2;

    buffer buf2 = buf1.copy();

    ASSERT_EQ(buf1.size(), buf2.size());
    ASSERT_NE(buf1.data(), buf2.data());
    ASSERT_EQ(buf1[0], buf2[0]);
    ASSERT_EQ(buf1[1], buf2[1]);
}

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

TEST(Buffer, MovesToAnotherObjectByMoveAssignment)
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
    *(buf.data()) = (std::byte)0x1;
    *(buf.data() + 1) = (std::byte)0x2;
    *(buf.data() + 2) = (std::byte)0x3;

    ASSERT_EQ(buf[2], (std::byte)0x3);
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
    *(buf.data()) = (std::byte)0x1;
    *(buf.data() + 1) = (std::byte)0x2;
    *(buf.data() + 2) = (std::byte)0x3;

    ASSERT_EQ(buf.at(2), (std::byte)0x3);
}

TEST(Buffer, ThrowsExceptionOnAttemptToGetElementByIndexWithBoundaryCheck)
{
    buffer buf(3);

    ASSERT_THROW(buf.at(10), std::out_of_range);
}

TEST(Buffer, IsAbleToIterateElementsLikeStandartContainer)
{
    buffer buf(3);
    *(buf.data()) = (std::byte)0x0;
    *(buf.data() + 1) = (std::byte)(0x1);
    *(buf.data() + 2) = (std::byte)(0x2);

    unsigned char byte_val = 0x0;
    for(auto byte : buf) {
        ASSERT_EQ((std::byte)(byte_val++), byte);
    }
}

TEST(Buffer, MayBeCheckedForEquality)
{
    buffer buf1(3);
    *(buf1.data()) = (std::byte)0x0;
    *(buf1.data() + 1) = (std::byte)(0x1);
    *(buf1.data() + 2) = (std::byte)(0x2);

    buffer buf2 = buf1.copy();

    ASSERT_TRUE(buf1 == buf2);
    ASSERT_TRUE(buf2 == buf1);
}

TEST(Buffer, MayBeCheckedForInequality)
{
    buffer buf1(1);
    buf1[0] = (std::byte)0x0;

    buffer buf2(2);
    buf2[0] = (std::byte)0x0;
    buf2[1] = (std::byte)0x1;
    buffer buf3(1);
    buf2[0] = (std::byte)0x1;

    ASSERT_TRUE(buf1 != buf2);
    ASSERT_TRUE(buf1 != buf3);
    ASSERT_TRUE(buf2 != buf3);
    ASSERT_TRUE(buf2 != buf1);
    ASSERT_TRUE(buf3 != buf1);
    ASSERT_TRUE(buf3 != buf2);
}

TEST(Buffer, HasConstIterators)
{
    buffer buf(3);
    *(buf.data()) = (std::byte)0x0;
    *(buf.data() + 1) = (std::byte)(0x1);
    *(buf.data() + 2) = (std::byte)(0x2);

    unsigned char byte_val = 0x0;
    for(auto it = buf.cbegin(); it != buf.cend(); ++it) {
        ASSERT_EQ((std::byte)(byte_val++), *it);
    }
}

TEST(Buffer, HasReverseIterators)
{
    buffer buf(3);
    *(buf.data()) = (std::byte)0x0;
    *(buf.data() + 1) = (std::byte)(0x1);
    *(buf.data() + 2) = (std::byte)(0x2);

    unsigned char byte_val = 0x2;
    for(auto it = buf.rbegin(); it != buf.rend(); ++it, --byte_val) {
        ASSERT_EQ((std::byte)byte_val, *it);
    }
}

TEST(Buffer, HasConstReverseIterators)
{
    buffer buf(3);
    *(buf.data()) = (std::byte)0x0;
    *(buf.data() + 1) = (std::byte)(0x1);
    *(buf.data() + 2) = (std::byte)(0x2);

    unsigned char byte_val = 0x2;
    for(auto it = buf.crbegin(); it != buf.crend(); ++it, --byte_val) {
        ASSERT_EQ((std::byte)byte_val, *it);
    }
}

TEST(BufferIterator, HasIteratorTraits)
{
    bool is_difference_type_same = std::is_same_v<buffer::iterator::difference_type, std::ptrdiff_t>;
    bool is_value_type_type = std::is_same_v<buffer::iterator::value_type, std::byte>;
    bool is_pointer_type_same = std::is_same_v<buffer::iterator::pointer, std::byte *>;
    bool is_reference_type_same = std::is_same_v<buffer::iterator::reference, std::byte &>;
    bool is_iterator_category_same = std::is_same_v<buffer::iterator::iterator_category,
                                                    std::random_access_iterator_tag>;
    
    ASSERT_TRUE(is_difference_type_same);
    ASSERT_TRUE(is_value_type_type);
    ASSERT_TRUE(is_pointer_type_same);
    ASSERT_TRUE(is_reference_type_same);
    ASSERT_TRUE(is_iterator_category_same);
}

TEST(BufferIterator, IncrementsIteratorWithPrefixIncrementation)
{
    buffer buf(3);
    *(buf.data()) = (std::byte)0x1;
    *(buf.data() + 1) = (std::byte)0x2;

    auto iter = buf.begin();
    ASSERT_EQ(*++iter, (std::byte)0x2);
}

TEST(BufferIterator, IncrementsIteratorWithPostfixIncrementation)
{
    buffer buf(3);
    *(buf.data()) = (std::byte)0x1;
    *(buf.data() + 1) = (std::byte)0x2;

    auto iter = buf.begin();
    auto prev_iter = iter++;

    ASSERT_EQ(*prev_iter, (std::byte)0x1);
    ASSERT_EQ(*iter, (std::byte)0x2);
}

TEST(BufferIterator, DecrementsIteratorWithPrefixDecrementation)
{
    buffer buf(2);
    *(buf.data()) = (std::byte)0x1;
    *(buf.data() + 1) = (std::byte)0x2;

    auto iter = ++buf.begin();
    ASSERT_EQ(*--iter, (std::byte)0x1);
}

TEST(BufferIterator, DecrementsIteratorWithPostfixDecrementation)
{
    buffer buf(3);
    *(buf.data()) = (std::byte)0x1;
    *(buf.data() + 1) = (std::byte)0x2;

    auto iter = ++buf.begin();
    auto prev_iter = iter--;

    ASSERT_EQ(*prev_iter, (std::byte)0x2);
    ASSERT_EQ(*iter, (std::byte)0x1);
}

TEST(BufferIterator, AllowsDereferencing)
{
    buffer buf(3);
    *(buf.data()) = (std::byte)0x1;

    ASSERT_EQ(*buf.begin(), (std::byte)0x1);
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

TEST(BufferIterator, SummarizeAnIteratorWithANumber)
{
    buffer buf(3);

    ASSERT_EQ(buf.begin() + 3, buf.end());
    ASSERT_EQ(3 + buf.begin(), buf.end());
}

TEST(BufferConstIterator, HasIteratorTraits)
{
    bool is_difference_type_same = std::is_same_v<buffer::const_iterator::difference_type, std::ptrdiff_t>;
    bool is_value_type_type = std::is_same_v<buffer::const_iterator::value_type, std::byte>;
    bool is_pointer_type_same = std::is_same_v<buffer::const_iterator::pointer, const std::byte *>;
    bool is_reference_type_same = std::is_same_v<buffer::const_iterator::reference, const std::byte &>;
    bool is_iterator_category_same = std::is_same_v<buffer::const_iterator::iterator_category,
                                                    std::random_access_iterator_tag>;

    ASSERT_TRUE(is_difference_type_same);
    ASSERT_TRUE(is_value_type_type);
    ASSERT_TRUE(is_pointer_type_same);
    ASSERT_TRUE(is_reference_type_same);
    ASSERT_TRUE(is_iterator_category_same);
}

TEST(BufferConstIterator, IncrementsIteratorWithPrefixIncrementation)
{
    buffer buf(3);
    *(buf.data()) = (std::byte)0x1;
    *(buf.data() + 1) = (std::byte)0x2;

    auto iter = buf.cbegin();
    ASSERT_EQ(*++iter, (std::byte)0x2);
}

TEST(BufferConstIterator, IncrementsIteratorWithPostfixIncrementation)
{
    buffer buf(3);
    *(buf.data()) = (std::byte)0x1;
    *(buf.data() + 1) = (std::byte)0x2;

    auto iter = buf.cbegin();
    auto prev_iter = iter++;

    ASSERT_EQ(*prev_iter, (std::byte)0x1);
    ASSERT_EQ(*iter, (std::byte)0x2);
}

TEST(BufferConstIterator, DecrementsIteratorWithPrefixDecrementation)
{
    buffer buf(3);
    *(buf.data()) = (std::byte)0x1;
    *(buf.data() + 1) = (std::byte)0x2;

    auto iter = ++buf.cbegin();
    ASSERT_EQ(*--iter, (std::byte)0x1);
}

TEST(BufferConstIterator, DecrementsIteratorWithPostfixDecrementation)
{
    buffer buf(3);
    *(buf.data()) = (std::byte)0x1;
    *(buf.data() + 1) = (std::byte)0x2;

    auto iter = ++buf.cbegin();
    auto prev_iter = iter--;

    ASSERT_EQ(*prev_iter, (std::byte)0x2);
    ASSERT_EQ(*iter, (std::byte)0x1);
}

TEST(BufferConstIterator, AllowsDereferencing)
{
    buffer buf(3);
    *(buf.data()) = (std::byte)0x1;

    ASSERT_EQ(*buf.cbegin(), (std::byte)0x1);
}

TEST(BufferConstIterator, DetermineEqualIterators)
{
    buffer buf(3);

    ASSERT_TRUE(buf.cbegin() == buf.cbegin());
}

TEST(BufferConstIterator, DetermineNonEqualIterators)
{
    buffer buf(3);

    ASSERT_TRUE(buf.cbegin() != buf.cend());
}

TEST(BufferConstIterator, SubtractOneIteratorFromAnother)
{
    buffer buf(3);

    ASSERT_EQ(buf.cend() - buf.cbegin(), 3);
}

TEST(BufferConstIterator, SubtractNumberFromIterator)
{
    buffer buf(3);

    ASSERT_EQ(buf.cend() - 3, buf.cbegin());
}

TEST(BufferConstIterator, SummarizeAnIteratorWithANumber)
{
    buffer buf(3);

    ASSERT_EQ(buf.cbegin() + 3, buf.cend());
    ASSERT_EQ(3 + buf.cbegin(), buf.cend());
}

TEST(BufferReverseIterator, HasIteratorTraits)
{
    bool is_difference_type_same = std::is_same_v<buffer::reverse_iterator::difference_type, std::ptrdiff_t>;
    bool is_value_type_type = std::is_same_v<buffer::reverse_iterator::value_type, std::byte>;
    bool is_pointer_type_same = std::is_same_v<buffer::reverse_iterator::pointer, std::byte *>;
    bool is_reference_type_same = std::is_same_v<buffer::reverse_iterator::reference, std::byte &>;
    bool is_iterator_category_same = std::is_same_v<buffer::reverse_iterator::iterator_category,
                                                    std::random_access_iterator_tag>;

    ASSERT_TRUE(is_difference_type_same);
    ASSERT_TRUE(is_value_type_type);
    ASSERT_TRUE(is_pointer_type_same);
    ASSERT_TRUE(is_reference_type_same);
    ASSERT_TRUE(is_iterator_category_same);
}

TEST(BufferReverseIterator, IncrementsIteratorWithPrefixIncrementation)
{
    buffer buf(2);
    *(buf.data()) = (std::byte)0x1;
    *(buf.data() + 1) = (std::byte)0x2;

    auto iter = buf.rbegin();
    ASSERT_EQ(*++iter, (std::byte)0x1);
}

TEST(BufferReverseIterator, IncrementsIteratorWithPostfixIncrementation)
{
    buffer buf(2);
    *(buf.data()) = (std::byte)0x1;
    *(buf.data() + 1) = (std::byte)0x2;

    auto iter = buf.rbegin();
    auto prev_iter = iter++;

    ASSERT_EQ(*prev_iter, (std::byte)0x2);
    ASSERT_EQ(*iter, (std::byte)0x1);
}

TEST(BufferReverseIterator, DecrementsIteratorWithPrefixDecrementation)
{
    buffer buf(2);
    *(buf.data()) = (std::byte)0x1;
    *(buf.data() + 1) = (std::byte)0x2;

    auto iter = ++buf.rbegin();
    ASSERT_EQ(*--iter, (std::byte)0x2);
}

TEST(BufferReverseIterator, DecrementsIteratorWithPostfixDecrementation)
{
    buffer buf(2);
    *(buf.data()) = (std::byte)0x1;
    *(buf.data() + 1) = (std::byte)0x2;

    auto iter = ++buf.rbegin();
    auto prev_iter = iter--;

    ASSERT_EQ(*prev_iter, (std::byte)0x1);
    ASSERT_EQ(*iter, (std::byte)0x2);
}

TEST(BufferReverseIterator, AllowsDereferencing)
{
    buffer buf(1);
    *(buf.data()) = (std::byte)0x1;

    ASSERT_EQ(*buf.rbegin(), (std::byte)0x1);
}

TEST(BufferReverseIterator, DetermineEqualIterators)
{
    buffer buf(3);

    ASSERT_TRUE(buf.rbegin() == buf.rbegin());
}

TEST(BufferReverseIterator, DetermineNonEqualIterators)
{
    buffer buf(3);

    ASSERT_TRUE(buf.rbegin() != buf.rend());
}

TEST(BufferReverseIterator, SubtractOneIteratorFromAnother)
{
    buffer buf(3);

    ASSERT_EQ(buf.rend() - buf.rbegin(), 3);
}

TEST(BufferReverseIterator, SubtractNumberFromIterator)
{
    buffer buf(3);

    ASSERT_EQ(buf.rend() - 3, buf.rbegin());
}

TEST(BufferReverseIterator, SummarizeAnIteratorWithANumber)
{
    buffer buf(3);

    ASSERT_EQ(buf.rbegin() + 3, buf.rend());
    ASSERT_EQ(3 + buf.rbegin(), buf.rend());
}

TEST(BufferReverseIterator, ConvertsToIterator)
{
    buffer buf(1);

    ASSERT_EQ(buf.begin() + 1, buf.rbegin().base());
}

TEST(BufferConstReverseIterator, HasIteratorTraits)
{
    bool is_difference_type_same = std::is_same_v<buffer::const_reverse_iterator::difference_type, std::ptrdiff_t>;
    bool is_value_type_type = std::is_same_v<buffer::const_reverse_iterator::value_type, std::byte>;
    bool is_pointer_type_same = std::is_same_v<buffer::const_reverse_iterator::pointer, const std::byte *>;
    bool is_reference_type_same = std::is_same_v<buffer::const_reverse_iterator::reference, const std::byte &>;
    bool is_iterator_category_same = std::is_same_v<buffer::const_reverse_iterator::iterator_category,
                                                    std::random_access_iterator_tag>;

    ASSERT_TRUE(is_difference_type_same);
    ASSERT_TRUE(is_value_type_type);
    ASSERT_TRUE(is_pointer_type_same);
    ASSERT_TRUE(is_reference_type_same);
    ASSERT_TRUE(is_iterator_category_same);
}

TEST(BufferConstReverseIterator, IncrementsIteratorWithPrefixIncrementation)
{
    buffer buf(2);
    *(buf.data()) = (std::byte)0x1;
    *(buf.data() + 1) = (std::byte)0x2;

    auto iter = buf.crbegin();
    ASSERT_EQ(*++iter, (std::byte)0x1);
}

TEST(BufferConstReverseIterator, IncrementsIteratorWithPostfixIncrementation)
{
    buffer buf(2);
    *(buf.data()) = (std::byte)0x1;
    *(buf.data() + 1) = (std::byte)0x2;

    auto iter = buf.crbegin();
    auto prev_iter = iter++;

    ASSERT_EQ(*prev_iter, (std::byte)0x2);
    ASSERT_EQ(*iter, (std::byte)0x1);
}

TEST(BufferConstReverseIterator, DecrementsIteratorWithPrefixDecrementation)
{
    buffer buf(2);
    *(buf.data()) = (std::byte)0x1;
    *(buf.data() + 1) = (std::byte)0x2;

    auto iter = ++buf.crbegin();
    ASSERT_EQ(*--iter, (std::byte)0x2);
}

TEST(BufferConstReverseIterator, DecrementsIteratorWithPostfixDecrementation)
{
    buffer buf(2);
    *(buf.data()) = (std::byte)0x1;
    *(buf.data() + 1) = (std::byte)0x2;

    auto iter = ++buf.crbegin();
    auto prev_iter = iter--;

    ASSERT_EQ(*prev_iter, (std::byte)0x1);
    ASSERT_EQ(*iter, (std::byte)0x2);
}

TEST(BufferConstReverseIterator, AllowsDereferencing)
{
    buffer buf(1);
    *(buf.data()) = (std::byte)0x1;

    ASSERT_EQ(*buf.crbegin(), (std::byte)0x1);
}

TEST(BufferConstReverseIterator, DetermineEqualIterators)
{
    buffer buf(3);

    ASSERT_TRUE(buf.crbegin() == buf.crbegin());
}

TEST(BufferConstReverseIterator, DetermineNonEqualIterators)
{
    buffer buf(3);

    ASSERT_TRUE(buf.crbegin() != buf.crend());
}

TEST(BufferConstReverseIterator, SubtractOneIteratorFromAnother)
{
    buffer buf(3);

    ASSERT_EQ(buf.crend() - buf.crbegin(), 3);
}

TEST(BufferConstReverseIterator, SubtractNumberFromIterator)
{
    buffer buf(3);

    ASSERT_EQ(buf.crend() - 3, buf.crbegin());
}

TEST(BufferConstReverseIterator, SummarizeAnIteratorWithANumber)
{
    buffer buf(3);

    ASSERT_EQ(buf.crbegin() + 3, buf.crend());
    ASSERT_EQ(3 + buf.crbegin(), buf.crend());
}

TEST(BufferConstReverseIterator, ConvertsToIterator)
{
    const buffer buf(1);

    ASSERT_EQ(buf.cbegin() + 1, buf.crbegin().base());
}
