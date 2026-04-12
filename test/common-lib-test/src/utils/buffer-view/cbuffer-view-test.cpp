#include <common-lib/utils/buffer-view/cbuffer-view.h>

#include <gtest/gtest.h>

using namespace vshalygin::cl;
using namespace testing;

TEST(BBufferView, MayBeCreatedFromBuffer)
{
    buffer buf(3);

    cbuffer_view sut = buf;

    ASSERT_EQ(sut.data(), buf.data());
}


TEST(CBufferView, ReturnsStoringData)
{
    std::vector<std::byte> buffer{ (std::byte)0x34, (std::byte)0x22 };

    cbuffer_view sut(buffer.data(), buffer.size());

    ASSERT_EQ(sut.data(), buffer.data());
}

TEST(CBufferView, ReturnsSizeOfStoringBuffer)
{
    std::vector<std::byte> buffer{ (std::byte)0x34, (std::byte)0x22 };

    const cbuffer_view sut(buffer.data(), buffer.size());

    ASSERT_EQ(sut.size(), buffer.size());
}

TEST(CBufferView, GetAccessToElementByIndex)
{
    std::vector<std::byte> buf{ (std::byte)0x0, (std::byte)0x1, (std::byte)0x34 };
    cbuffer_view sut(buf.data(), buf.size());

    auto third_byte = sut[2];

    ASSERT_EQ(third_byte, (std::byte)0x34);
}

TEST(CBufferView, ConvertsToFalseIfEmpty)
{
    cbuffer_view buf_view;

    ASSERT_FALSE(buf_view);
}

TEST(CBufferView, ConvertsToTrueIfNotEmpty)
{
    std::vector<std::byte> buf{ (std::byte)0x0, (std::byte)0x1, (std::byte)0x2 };
    cbuffer_view buf_view(buf.data(), buf.size());

    ASSERT_TRUE(buf_view);
}

TEST(CBufferView, GivesAccessToTheElementByIndexWithBoundaryCheck)
{
    std::vector<std::byte> buf{ (std::byte)0x1, (std::byte)0x2, (std::byte)0x3 };
    cbuffer_view buf_view(buf.data(), buf.size());

    ASSERT_EQ(buf_view.at(2), (std::byte)0x3);
}

TEST(CBufferView, ThrowsExceptionOnAttemptToGetElementByIndexWithBoundaryCheck)
{
    std::vector<std::byte> buf{ (std::byte)0x0, (std::byte)0x1, (std::byte)0x2 };
    cbuffer_view buf_view(buf.data(), buf.size());

    ASSERT_THROW(buf_view.at(10), std::out_of_range);
}

TEST(CBufferView, IsAbleToIterateElementsLikeStandartContainer)
{
    std::vector<std::byte> buf{ (std::byte)0x0, (std::byte)0x1, (std::byte)0x2 };
    cbuffer_view sut(buf.data(), buf.size());

    unsigned char byte_val = 0x0;
    for(auto byte : sut) {
        ASSERT_EQ((std::byte)(byte_val++), byte);
    }
}

TEST(CBufferView, HasConstIterators)
{
    std::vector<std::byte> buffer{ (std::byte)0x0, (std::byte)0x1, (std::byte)0x2 };
    cbuffer_view sut(buffer.data(), buffer.size());

    unsigned char byte_val = 0x0;
    for(auto it = sut.cbegin(); it != sut.cend(); ++it) {
        ASSERT_EQ((std::byte)(byte_val++), *it);
    }
}

TEST(CBufferView, HasReverseIterators)
{
    std::vector<std::byte> buffer{ (std::byte)0x0, (std::byte)0x1, (std::byte)0x2 };
    cbuffer_view sut(buffer.data(), buffer.size());

    unsigned char byte_val = 0x2;
    for(auto it = sut.rbegin(); it != sut.rend(); ++it, --byte_val) {
        ASSERT_EQ((std::byte)byte_val, *it);
    }
}

TEST(CBufferView, HasConstReverseIterators)
{
    std::vector<std::byte> buffer{ (std::byte)0x0, (std::byte)0x1, (std::byte)0x2 };
    cbuffer_view sut(buffer.data(), buffer.size());

    unsigned char byte_val = 0x2;
    for(auto it = sut.crbegin(); it != sut.crend(); ++it, --byte_val) {
        ASSERT_EQ((std::byte)byte_val, *it);
    }
}

TEST(CBufferView, MayBeCheckedForEquality)
{
    std::vector<std::byte> buffer1{ (std::byte)0x0, (std::byte)0x1, (std::byte)0x2 };
    std::vector<std::byte> buffer2{ (std::byte)0x0, (std::byte)0x1, (std::byte)0x2 };
    cbuffer_view buf1(buffer1.data(), buffer1.size());
    cbuffer_view buf2(buffer2.data(), buffer2.size());

    ASSERT_TRUE(buf1 == buf2);
    ASSERT_TRUE(buf2 == buf1);
}

TEST(CBufferView, MayBeCheckedForInequality)
{
    std::vector<std::byte> buffer1{ (std::byte)0x0 };
    std::vector<std::byte> buffer2{ (std::byte)0x0, (std::byte)0x1 };
    std::vector<std::byte> buffer3{ (std::byte)0x1 };
    cbuffer_view buf1(buffer1.data(), buffer1.size());
    cbuffer_view buf2(buffer2.data(), buffer2.size());
    cbuffer_view buf3(buffer3.data(), buffer3.size());

    ASSERT_TRUE(buf1 != buf2);
    ASSERT_TRUE(buf1 != buf3);
    ASSERT_TRUE(buf2 != buf3);
    ASSERT_TRUE(buf2 != buf1);
    ASSERT_TRUE(buf3 != buf1);
    ASSERT_TRUE(buf3 != buf2);
}

TEST(CBufferViewIterator, CBufferVieweratorTraits)
{
    bool is_difference_type_same = std::is_same_v<cbuffer_view::iterator::difference_type, std::ptrdiff_t>;
    bool is_value_type_type = std::is_same_v<cbuffer_view::iterator::value_type, std::byte>;
    bool is_pointer_type_same = std::is_same_v<cbuffer_view::iterator::pointer, const std::byte *>;
    bool is_reference_type_same = std::is_same_v<cbuffer_view::iterator::reference, const std::byte &>;
    bool is_iterator_category_same = std::is_same_v<cbuffer_view::iterator::iterator_category,
        std::random_access_iterator_tag>;

    ASSERT_TRUE(is_difference_type_same);
    ASSERT_TRUE(is_value_type_type);
    ASSERT_TRUE(is_pointer_type_same);
    ASSERT_TRUE(is_reference_type_same);
    ASSERT_TRUE(is_iterator_category_same);
}

TEST(CBufferViewIterator, IncrementsIteratorWithPrefixIncrementation)
{
    std::vector<std::byte> buffer{ (std::byte)0x1, (std::byte)0x2, (std::byte)0x3 };
    cbuffer_view sut(buffer.data(), buffer.size());

    auto iter = sut.begin();
    ASSERT_EQ(*++iter, (std::byte)0x2);
}

TEST(CBufferViewIterator, IncrementsIteratorWithPostfixIncrementation)
{
    std::vector<std::byte> buffer{ (std::byte)0x1, (std::byte)0x2, (std::byte)0x3 };
    cbuffer_view sut(buffer.data(), buffer.size());

    auto iter = sut.begin();
    auto prev_iter = iter++;

    ASSERT_EQ(*prev_iter, (std::byte)0x1);
    ASSERT_EQ(*iter, (std::byte)0x2);
}

TEST(CBufferViewIterator, DecrementsIteratorWithPrefixDecrementation)
{
    std::vector<std::byte> buffer{ (std::byte)0x1, (std::byte)0x2 };
    cbuffer_view sut(buffer.data(), buffer.size());

    auto iter = ++sut.begin();
    ASSERT_EQ(*--iter, (std::byte)0x1);
}

TEST(CBufferViewIterator, DecrementsIteratorWithPostfixDecrementation)
{
    std::vector<std::byte> buffer{ (std::byte)0x1, (std::byte)0x2, (std::byte)0x3 };
    cbuffer_view sut(buffer.data(), buffer.size());

    auto iter = ++sut.begin();
    auto prev_iter = iter--;

    ASSERT_EQ(*prev_iter, (std::byte)0x2);
    ASSERT_EQ(*iter, (std::byte)0x1);
}

TEST(CBufferViewIterator, AllowsDereferencing)
{
    std::vector<std::byte> buffer{ (std::byte)0x1, (std::byte)0x2, (std::byte)0x3 };
    cbuffer_view sut(buffer.data(), buffer.size());

    ASSERT_EQ(*sut.begin(), (std::byte)0x1);
}

TEST(CBufferViewIterator, DetermineEqualIterators)
{
    std::vector<std::byte> buffer{ (std::byte)0x0, (std::byte)0x1, (std::byte)0x2 };
    cbuffer_view sut(buffer.data(), buffer.size());

    ASSERT_TRUE(sut.begin() == sut.begin());
}

TEST(CBufferViewIterator, DetermineNonEqualIterators)
{
    std::vector<std::byte> buffer{ (std::byte)0x0, (std::byte)0x1, (std::byte)0x2 };
    cbuffer_view sut(buffer.data(), buffer.size());

    ASSERT_TRUE(sut.begin() != sut.end());
}

TEST(CBufferViewIterator, SubtractOneIteratorFromAnother)
{
    std::vector<std::byte> buffer{ (std::byte)0x0, (std::byte)0x1, (std::byte)0x2 };
    cbuffer_view sut(buffer.data(), buffer.size());

    ASSERT_EQ(sut.end() - sut.begin(), 3);
}

TEST(CBufferViewViewIterator, SubtractNumberFromIterator)
{
    std::vector<std::byte> buffer{ (std::byte)0x0, (std::byte)0x1, (std::byte)0x2 };
    cbuffer_view sut(buffer.data(), buffer.size());

    ASSERT_EQ(sut.end() - 3, sut.begin());
}

TEST(CBufferViewIterator, SummarizeAnIteratorWithANumber)
{
    std::vector<std::byte> buffer{ (std::byte)0x0, (std::byte)0x1, (std::byte)0x2 };
    cbuffer_view sut(buffer.data(), buffer.size());

    ASSERT_EQ(sut.begin() + 3, sut.end());
    ASSERT_EQ(3 + sut.begin(), sut.end());
}

TEST(CBufferViewConstIterator, CBufferVieweratorTraits)
{
    bool is_difference_type_same = std::is_same_v<cbuffer_view::const_iterator::difference_type, std::ptrdiff_t>;
    bool is_value_type_type = std::is_same_v<cbuffer_view::const_iterator::value_type, std::byte>;
    bool is_pointer_type_same = std::is_same_v<cbuffer_view::const_iterator::pointer, const std::byte *>;
    bool is_reference_type_same = std::is_same_v<cbuffer_view::const_iterator::reference, const std::byte &>;
    bool is_iterator_category_same = std::is_same_v<cbuffer_view::const_iterator::iterator_category,
        std::random_access_iterator_tag>;

    ASSERT_TRUE(is_difference_type_same);
    ASSERT_TRUE(is_value_type_type);
    ASSERT_TRUE(is_pointer_type_same);
    ASSERT_TRUE(is_reference_type_same);
    ASSERT_TRUE(is_iterator_category_same);
}

TEST(CBufferViewConstIterator, IncrementsIteratorWithPrefixIncrementation)
{
    std::vector<std::byte> buffer{ (std::byte)0x1, (std::byte)0x2, (std::byte)0x3 };
    cbuffer_view sut(buffer.data(), buffer.size());

    auto iter = sut.cbegin();
    ASSERT_EQ(*++iter, (std::byte)0x2);
}

TEST(CBufferViewConstIterator, IncrementsIteratorWithPostfixIncrementation)
{
    std::vector<std::byte> buffer{ (std::byte)0x1, (std::byte)0x2, (std::byte)0x3 };
    cbuffer_view sut(buffer.data(), buffer.size());

    auto iter = sut.cbegin();
    auto prev_iter = iter++;

    ASSERT_EQ(*prev_iter, (std::byte)0x1);
    ASSERT_EQ(*iter, (std::byte)0x2);
}

TEST(CBufferViewConstIterator, DecrementsIteratorWithPrefixDecrementation)
{
    std::vector<std::byte> buffer{ (std::byte)0x1, (std::byte)0x2, (std::byte)0x3 };
    cbuffer_view sut(buffer.data(), buffer.size());

    auto iter = ++sut.cbegin();
    ASSERT_EQ(*--iter, (std::byte)0x1);
}

TEST(CBufferViewConstIterator, DecrementsIteratorWithPostfixDecrementation)
{
    std::vector<std::byte> buffer{ (std::byte)0x1, (std::byte)0x2, (std::byte)0x3 };
    cbuffer_view sut(buffer.data(), buffer.size());

    auto iter = ++sut.cbegin();
    auto prev_iter = iter--;

    ASSERT_EQ(*prev_iter, (std::byte)0x2);
    ASSERT_EQ(*iter, (std::byte)0x1);
}

TEST(CBufferViewConstIterator, AllowsDereferencing)
{
    std::vector<std::byte> buffer{ (std::byte)0x1, (std::byte)0x2, (std::byte)0x3 };
    cbuffer_view sut(buffer.data(), buffer.size());

    ASSERT_EQ(*sut.cbegin(), (std::byte)0x1);
}

TEST(CBufferViewConstIterator, DetermineEqualIterators)
{
    std::vector<std::byte> buffer{ (std::byte)0x0, (std::byte)0x1, (std::byte)0x2 };
    cbuffer_view sut(buffer.data(), buffer.size());

    ASSERT_TRUE(sut.cbegin() == sut.cbegin());
}

TEST(CBufferViewConstIterator, DetermineNonEqualIterators)
{
    std::vector<std::byte> buffer{ (std::byte)0x0, (std::byte)0x1, (std::byte)0x2 };
    cbuffer_view sut(buffer.data(), buffer.size());

    ASSERT_TRUE(sut.cbegin() != sut.cend());
}

TEST(CBufferViewConstIterator, SubtractOneIteratorFromAnother)
{
    std::vector<std::byte> buffer{ (std::byte)0x0, (std::byte)0x1, (std::byte)0x2 };
    cbuffer_view sut(buffer.data(), buffer.size());

    ASSERT_EQ(sut.cend() - sut.cbegin(), 3);
}

TEST(CBufferViewConstIterator, SubtractNumberFromIterator)
{
    std::vector<std::byte> buffer{ (std::byte)0x0, (std::byte)0x1, (std::byte)0x2 };
    cbuffer_view sut(buffer.data(), buffer.size());

    ASSERT_EQ(sut.cend() - 3, sut.cbegin());
}

TEST(CBufferViewConstIterator, SummarizeAnIteratorWithANumber)
{
    std::vector<std::byte> buffer{ (std::byte)0x0, (std::byte)0x1, (std::byte)0x2 };
    cbuffer_view sut(buffer.data(), buffer.size());

    ASSERT_EQ(sut.cbegin() + 3, sut.cend());
    ASSERT_EQ(3 + sut.cbegin(), sut.cend());
}

TEST(CBufferViewReverseIterator, CBufferVieweratorTraits)
{
    bool is_difference_type_same = std::is_same_v<cbuffer_view::reverse_iterator::difference_type, std::ptrdiff_t>;
    bool is_value_type_type = std::is_same_v<cbuffer_view::reverse_iterator::value_type, std::byte>;
    bool is_pointer_type_same = std::is_same_v<cbuffer_view::reverse_iterator::pointer, const std::byte *>;
    bool is_reference_type_same = std::is_same_v<cbuffer_view::reverse_iterator::reference, const std::byte &>;
    bool is_iterator_category_same = std::is_same_v<cbuffer_view::reverse_iterator::iterator_category,
        std::random_access_iterator_tag>;

    ASSERT_TRUE(is_difference_type_same);
    ASSERT_TRUE(is_value_type_type);
    ASSERT_TRUE(is_pointer_type_same);
    ASSERT_TRUE(is_reference_type_same);
    ASSERT_TRUE(is_iterator_category_same);
}

TEST(CBufferViewReverseIterator, IncrementsIteratorWithPrefixIncrementation)
{
    std::vector<std::byte> buffer{ (std::byte)0x1, (std::byte)0x2 };
    cbuffer_view sut(buffer.data(), buffer.size());

    auto iter = sut.rbegin();
    ASSERT_EQ(*++iter, (std::byte)0x1);
}

TEST(CBufferViewReverseIterator, IncrementsIteratorWithPostfixIncrementation)
{
    std::vector<std::byte> buffer{ (std::byte)0x1, (std::byte)0x2 };
    cbuffer_view sut(buffer.data(), buffer.size());

    auto iter = sut.rbegin();
    auto prev_iter = iter++;

    ASSERT_EQ(*prev_iter, (std::byte)0x2);
    ASSERT_EQ(*iter, (std::byte)0x1);
}

TEST(CBufferViewReverseIterator, DecrementsIteratorWithPrefixDecrementation)
{
    std::vector<std::byte> buffer{ (std::byte)0x1, (std::byte)0x2 };
    cbuffer_view sut(buffer.data(), buffer.size());

    auto iter = ++sut.rbegin();
    ASSERT_EQ(*--iter, (std::byte)0x2);
}

TEST(CBufferViewReverseIterator, DecrementsIteratorWithPostfixDecrementation)
{
    std::vector<std::byte> buffer{ (std::byte)0x1, (std::byte)0x2 };
    cbuffer_view sut(buffer.data(), buffer.size());

    auto iter = ++sut.rbegin();
    auto prev_iter = iter--;

    ASSERT_EQ(*prev_iter, (std::byte)0x1);
    ASSERT_EQ(*iter, (std::byte)0x2);
}

TEST(CBufferViewReverseIterator, AllowsDereferencing)
{
    std::vector<std::byte> buffer{ (std::byte)0x1 };
    cbuffer_view sut(buffer.data(), buffer.size());

    ASSERT_EQ(*sut.rbegin(), (std::byte)0x1);
}

TEST(CBufferViewReverseIterator, DetermineEqualIterators)
{
    std::vector<std::byte> buffer{ (std::byte)0x0, (std::byte)0x1, (std::byte)0x3 };
    cbuffer_view sut(buffer.data(), buffer.size());

    ASSERT_TRUE(sut.rbegin() == sut.rbegin());
}

TEST(CBufferViewReverseIterator, DetermineNonEqualIterators)
{
    std::vector<std::byte> buffer{ (std::byte)0x0, (std::byte)0x1, (std::byte)0x3 };
    cbuffer_view sut(buffer.data(), buffer.size());

    ASSERT_TRUE(sut.rbegin() != sut.rend());
}

TEST(CBufferViewReverseIterator, SubtractOneIteratorFromAnother)
{
    std::vector<std::byte> buffer{ (std::byte)0x0, (std::byte)0x1, (std::byte)0x3 };
    cbuffer_view sut(buffer.data(), buffer.size());

    ASSERT_EQ(sut.rend() - sut.rbegin(), 3);
}

TEST(CBufferViewReverseIterator, SubtractNumberFromIterator)
{
    std::vector<std::byte> buffer{ (std::byte)0x0, (std::byte)0x1, (std::byte)0x3 };
    cbuffer_view sut(buffer.data(), buffer.size());

    ASSERT_EQ(sut.rend() - 3, sut.rbegin());
}

TEST(CBufferViewReverseIterator, SummarizeAnIteratorWithANumber)
{
    std::vector<std::byte> buffer{ (std::byte)0x0, (std::byte)0x1, (std::byte)0x3 };
    cbuffer_view sut(buffer.data(), buffer.size());

    ASSERT_EQ(sut.rbegin() + 3, sut.rend());
    ASSERT_EQ(3 + sut.rbegin(), sut.rend());
}

TEST(CBufferViewReverseIterator, ConvertsToIterator)
{
    std::vector<std::byte> buffer{ (std::byte)0x0 };
    cbuffer_view sut(buffer.data(), buffer.size());

    ASSERT_EQ(sut.begin() + 1, sut.rbegin().base());
}

TEST(CBufferViewConstReverseIterator, CBufferVieweratorTraits)
{
    bool is_difference_type_same = std::is_same_v<cbuffer_view::const_reverse_iterator::difference_type, std::ptrdiff_t>;
    bool is_value_type_type = std::is_same_v<cbuffer_view::const_reverse_iterator::value_type, std::byte>;
    bool is_pointer_type_same = std::is_same_v<cbuffer_view::const_reverse_iterator::pointer, const std::byte *>;
    bool is_reference_type_same = std::is_same_v<cbuffer_view::const_reverse_iterator::reference, const std::byte &>;
    bool is_iterator_category_same = std::is_same_v<cbuffer_view::const_reverse_iterator::iterator_category,
        std::random_access_iterator_tag>;

    ASSERT_TRUE(is_difference_type_same);
    ASSERT_TRUE(is_value_type_type);
    ASSERT_TRUE(is_pointer_type_same);
    ASSERT_TRUE(is_reference_type_same);
    ASSERT_TRUE(is_iterator_category_same);
}

TEST(CBufferViewConstReverseIterator, IncrementsIteratorWithPrefixIncrementation)
{
    std::vector<std::byte> buffer{ (std::byte)0x1, (std::byte)0x2 };
    cbuffer_view sut(buffer.data(), buffer.size());

    auto iter = sut.crbegin();
    ASSERT_EQ(*++iter, (std::byte)0x1);
}

TEST(CBufferViewConstReverseIterator, IncrementsIteratorWithPostfixIncrementation)
{
    std::vector<std::byte> buffer{ (std::byte)0x1, (std::byte)0x2 };
    cbuffer_view sut(buffer.data(), buffer.size());

    auto iter = sut.crbegin();
    auto prev_iter = iter++;

    ASSERT_EQ(*prev_iter, (std::byte)0x2);
    ASSERT_EQ(*iter, (std::byte)0x1);
}

TEST(CBufferViewConstReverseIterator, DecrementsIteratorWithPrefixDecrementation)
{
    std::vector<std::byte> buffer{ (std::byte)0x1, (std::byte)0x2 };
    cbuffer_view sut(buffer.data(), buffer.size());

    auto iter = ++sut.crbegin();
    ASSERT_EQ(*--iter, (std::byte)0x2);
}

TEST(CBufferViewConstReverseIterator, DecrementsIteratorWithPostfixDecrementation)
{
    std::vector<std::byte> buffer{ (std::byte)0x1, (std::byte)0x2 };
    cbuffer_view sut(buffer.data(), buffer.size());

    auto iter = ++sut.crbegin();
    auto prev_iter = iter--;

    ASSERT_EQ(*prev_iter, (std::byte)0x1);
    ASSERT_EQ(*iter, (std::byte)0x2);
}

TEST(CBufferViewConstReverseIterator, AllowsDereferencing)
{
    std::vector<std::byte> buffer{ (std::byte)0x1, (std::byte)0x2 };
    cbuffer_view sut(buffer.data(), buffer.size());

    ASSERT_EQ(*sut.crbegin(), (std::byte)0x2);
}

TEST(CBufferViewConstReverseIterator, DetermineEqualIterators)
{
    std::vector<std::byte> buffer{ (std::byte)0x1, (std::byte)0x2, (std::byte)0x3 };
    cbuffer_view sut(buffer.data(), buffer.size());

    ASSERT_TRUE(sut.crbegin() == sut.crbegin());
}

TEST(CBufferViewConstReverseIterator, DetermineNonEqualIterators)
{
    std::vector<std::byte> buffer{ (std::byte)0x1, (std::byte)0x2, (std::byte)0x3 };
    cbuffer_view sut(buffer.data(), buffer.size());

    ASSERT_TRUE(sut.crbegin() != sut.crend());
}

TEST(CBufferViewConstReverseIterator, SubtractOneIteratorFromAnother)
{
    std::vector<std::byte> buffer{ (std::byte)0x1, (std::byte)0x2, (std::byte)0x3 };
    cbuffer_view sut(buffer.data(), buffer.size());

    ASSERT_EQ(sut.crend() - sut.crbegin(), 3);
}

TEST(CBufferViewConstReverseIterator, SubtractNumberFromIterator)
{
    std::vector<std::byte> buffer{ (std::byte)0x1, (std::byte)0x2, (std::byte)0x3 };
    cbuffer_view sut(buffer.data(), buffer.size());

    ASSERT_EQ(sut.crend() - 3, sut.crbegin());
}

TEST(CBufferViewConstReverseIterator, SummarizeAnIteratorWithANumber)
{
    std::vector<std::byte> buffer{ (std::byte)0x1, (std::byte)0x2, (std::byte)0x3 };
    cbuffer_view sut(buffer.data(), buffer.size());

    ASSERT_EQ(sut.crbegin() + 3, sut.crend());
    ASSERT_EQ(3 + sut.crbegin(), sut.crend());
}

TEST(CBufferViewConstReverseIterator, ConvertsToIterator)
{
    std::vector<std::byte> buffer{ (std::byte)0x1 };
    cbuffer_view sut(buffer.data(), buffer.size());

    ASSERT_EQ(sut.cbegin() + 1, sut.crbegin().base());
}
