#include <common-lib/utils/buffer-view/buffer-view.h>

#include <gtest/gtest.h>

using namespace vsh::common_lib;
using namespace testing;

TEST(BufferView, ReturnsStoringData)
{
    std::vector<unsigned char> buffer{ 0x34, 0x22 };

    buffer_view sut(buffer.data(), buffer.size());

    ASSERT_EQ(sut.data(), buffer.data());
}

TEST(BufferView, ReturnsStoringConstDataIfObjectIsConst)
{
    std::vector<unsigned char> buffer{ 0x34, 0x22 };

    const buffer_view sut(buffer.data(), buffer.size());

    ASSERT_EQ(sut.data(), buffer.data());
}

TEST(BufferView, ReturnsSizeOfStoringBuffer)
{
    std::vector<unsigned char> buffer{ 0x34, 0x22 };

    const buffer_view sut(buffer.data(), buffer.size());

    ASSERT_EQ(sut.size(), buffer.size());
}

TEST(BufferView, GetsAccessToElementByIndex)
{
    std::vector<unsigned char> buf{ 0x0, 0x1, 0x2 };
    buffer_view sut(buf.data(), buf.size());

    sut[2] = 0x34;

    ASSERT_EQ(sut[2], 0x34);
}

TEST(BufferView, ConvertsToFalseIfEmpty)
{
    buffer_view buf_view;

    ASSERT_FALSE(buf_view);
}

TEST(BufferView, ConvertsToTrueIfNotEmpty)
{
    std::vector<unsigned char> buf{ 0x0, 0x1, 0x2 };
    buffer_view buf_view(buf.data(), buf.size());

    ASSERT_TRUE(buf_view);
}

TEST(BufferView, GivesAccessToTheElementByIndexWithBoundaryCheck)
{
    std::vector<unsigned char> buf{ 0x1, 0x2, 0x3 };
    buffer_view buf_view(buf.data(), buf.size());

    ASSERT_EQ(buf_view.at(2), 0x3);
}

TEST(BufferView, ThrowsExceptionOnAttemptToGetElementByIndexWithBoundaryCheck)
{
    std::vector<unsigned char> buf{ 0x0, 0x1, 0x2 };
    buffer_view buf_view(buf.data(), buf.size());

    ASSERT_THROW(buf_view.at(10), std::out_of_range);
}

TEST(BufferView, IsAbleToIterateElementsLikeStandartContainer)
{
    std::vector<unsigned char> buf{ 0x0, 0x1, 0x2 };
    buffer_view sut(buf.data(), buf.size());

    unsigned char byte_val = 0x0;
    for(auto byte : sut) {
        ASSERT_EQ(byte_val++, byte);
    }
}

TEST(BufferView, HasConstIterators)
{
    std::vector<unsigned char> buffer{ 0x0, 0x1, 0x2 };
    buffer_view sut(buffer.data(), buffer.size());

    unsigned char byte_val = 0x0;
    for(auto it = sut.cbegin(); it != sut.cend(); ++it) {
        ASSERT_EQ(byte_val++, *it);
    }
}

TEST(BufferView, HasReverseIterators)
{
    std::vector<unsigned char> buffer{ 0x0, 0x1, 0x2 };
    buffer_view sut(buffer.data(), buffer.size());

    unsigned char byte_val = 0x2;
    for(auto it = sut.rbegin(); it != sut.rend(); ++it, --byte_val) {
        ASSERT_EQ(byte_val, *it);
    }
}

TEST(BufferView, HasConstReverseIterators)
{
    std::vector<unsigned char> buffer{ 0x0, 0x1, 0x2 };
    buffer_view sut(buffer.data(), buffer.size());

    unsigned char byte_val = 0x2;
    for(auto it = sut.crbegin(); it != sut.crend(); ++it, --byte_val) {
        ASSERT_EQ(byte_val, *it);
    }
}

TEST(BufferViewIterator, HasIteratorTraits)
{
    bool is_difference_type_same = std::is_same_v<buffer_view::iterator::difference_type, std::ptrdiff_t>;
    bool is_value_type_type = std::is_same_v<buffer_view::iterator::value_type, unsigned char>;
    bool is_pointer_type_same = std::is_same_v<buffer_view::iterator::pointer, unsigned char *>;
    bool is_reference_type_same = std::is_same_v<buffer_view::iterator::reference, unsigned char &>;
    bool is_iterator_category_same = std::is_same_v<buffer_view::iterator::iterator_category,
                                                    std::random_access_iterator_tag>;

    ASSERT_TRUE(is_difference_type_same);
    ASSERT_TRUE(is_value_type_type);
    ASSERT_TRUE(is_pointer_type_same);
    ASSERT_TRUE(is_reference_type_same);
    ASSERT_TRUE(is_iterator_category_same);
}

TEST(BufferViewIterator, IncrementsIteratorWithPrefixIncrementation)
{
    std::vector<unsigned char> buffer{ 0x1, 0x2, 0x3 };
    buffer_view sut(buffer.data(), buffer.size());

    auto iter = sut.begin();
    ASSERT_EQ(*++iter, 0x2);
}

TEST(BufferViewIterator, IncrementsIteratorWithPostfixIncrementation)
{
    std::vector<unsigned char> buffer{ 0x1, 0x2, 0x3 };
    buffer_view sut(buffer.data(), buffer.size());

    auto iter = sut.begin();
    auto prev_iter = iter++;

    ASSERT_EQ(*prev_iter, 0x1);
    ASSERT_EQ(*iter, 0x2);
}

TEST(BufferViewIterator, DecrementsIteratorWithPrefixDecrementation)
{
    std::vector<unsigned char> buffer{ 0x1, 0x2 };
    buffer_view sut(buffer.data(), buffer.size());

    auto iter = ++sut.begin();
    ASSERT_EQ(*--iter, 0x1);
}

TEST(BufferViewIterator, DecrementsIteratorWithPostfixDecrementation)
{
    std::vector<unsigned char> buffer{ 0x1, 0x2, 0x3 };
    buffer_view sut(buffer.data(), buffer.size());

    auto iter = ++sut.begin();
    auto prev_iter = iter--;

    ASSERT_EQ(*prev_iter, 0x2);
    ASSERT_EQ(*iter, 0x1);
}

TEST(BufferViewIterator, AllowsDereferencing)
{
    std::vector<unsigned char> buffer{ 0x1, 0x2, 0x3 };
    buffer_view sut(buffer.data(), buffer.size());

    ASSERT_EQ(*sut.begin(), 0x1);
}

TEST(BufferViewIterator, DetermineEqualIterators)
{
    std::vector<unsigned char> buffer{ 0x0, 0x1, 0x2 };
    buffer_view sut(buffer.data(), buffer.size());

    ASSERT_TRUE(sut.begin() == sut.begin());
}

TEST(BufferViewIterator, DetermineNonEqualIterators)
{
    std::vector<unsigned char> buffer{ 0x0, 0x1, 0x2 };
    buffer_view sut(buffer.data(), buffer.size());

    ASSERT_TRUE(sut.begin() != sut.end());
}

TEST(BufferViewIterator, SubtractOneIteratorFromAnother)
{
    std::vector<unsigned char> buffer{ 0x0, 0x1, 0x2 };
    buffer_view sut(buffer.data(), buffer.size());

    ASSERT_EQ(sut.end() - sut.begin(), 3);
}

TEST(BufferViewViewIterator, SubtractNumberFromIterator)
{
    std::vector<unsigned char> buffer{ 0x0, 0x1, 0x2 };
    buffer_view sut(buffer.data(), buffer.size());

    ASSERT_EQ(sut.end() - 3, sut.begin());
}

TEST(BufferViewIterator, SummarizeAnIteratorWithANumber)
{
    std::vector<unsigned char> buffer{ 0x0, 0x1, 0x2 };
    buffer_view sut(buffer.data(), buffer.size());

    ASSERT_EQ(sut.begin() + 3, sut.end());
    ASSERT_EQ(3 + sut.begin(), sut.end());
}

TEST(BufferViewConstIterator, HasIteratorTraits)
{
    bool is_difference_type_same = std::is_same_v<buffer_view::const_iterator::difference_type, std::ptrdiff_t>;
    bool is_value_type_type = std::is_same_v<buffer_view::const_iterator::value_type, unsigned char>;
    bool is_pointer_type_same = std::is_same_v<buffer_view::const_iterator::pointer, const unsigned char *>;
    bool is_reference_type_same = std::is_same_v<buffer_view::const_iterator::reference, const unsigned char &>;
    bool is_iterator_category_same = std::is_same_v<buffer_view::const_iterator::iterator_category,
                                                    std::random_access_iterator_tag>;

    ASSERT_TRUE(is_difference_type_same);
    ASSERT_TRUE(is_value_type_type);
    ASSERT_TRUE(is_pointer_type_same);
    ASSERT_TRUE(is_reference_type_same);
    ASSERT_TRUE(is_iterator_category_same);
}

TEST(BufferViewConstIterator, IncrementsIteratorWithPrefixIncrementation)
{
    std::vector<unsigned char> buffer{ 0x1, 0x2, 0x3 };
    buffer_view sut(buffer.data(), buffer.size());

    auto iter = sut.cbegin();
    ASSERT_EQ(*++iter, 0x2);
}

TEST(BufferViewConstIterator, IncrementsIteratorWithPostfixIncrementation)
{
    std::vector<unsigned char> buffer{ 0x1, 0x2, 0x3 };
    buffer_view sut(buffer.data(), buffer.size());

    auto iter = sut.cbegin();
    auto prev_iter = iter++;

    ASSERT_EQ(*prev_iter, 0x1);
    ASSERT_EQ(*iter, 0x2);
}

TEST(BufferViewConstIterator, DecrementsIteratorWithPrefixDecrementation)
{
    std::vector<unsigned char> buffer{ 0x1, 0x2, 0x3 };
    buffer_view sut(buffer.data(), buffer.size());

    auto iter = ++sut.cbegin();
    ASSERT_EQ(*--iter, 0x1);
}

TEST(BufferViewConstIterator, DecrementsIteratorWithPostfixDecrementation)
{
    std::vector<unsigned char> buffer{ 0x1, 0x2, 0x3 };
    buffer_view sut(buffer.data(), buffer.size());

    auto iter = ++sut.cbegin();
    auto prev_iter = iter--;

    ASSERT_EQ(*prev_iter, 0x2);
    ASSERT_EQ(*iter, 0x1);
}

TEST(BufferViewConstIterator, AllowsDereferencing)
{
    std::vector<unsigned char> buffer{ 0x1, 0x2, 0x3 };
    buffer_view sut(buffer.data(), buffer.size());

    ASSERT_EQ(*sut.cbegin(), 0x1);
}

TEST(BufferViewConstIterator, DetermineEqualIterators)
{
    std::vector<unsigned char> buffer{ 0x0, 0x1, 0x2 };
    buffer_view sut(buffer.data(), buffer.size());

    ASSERT_TRUE(sut.cbegin() == sut.cbegin());
}

TEST(BufferViewConstIterator, DetermineNonEqualIterators)
{
    std::vector<unsigned char> buffer{ 0x0, 0x1, 0x2 };
    buffer_view sut(buffer.data(), buffer.size());

    ASSERT_TRUE(sut.cbegin() != sut.cend());
}

TEST(BufferViewConstIterator, SubtractOneIteratorFromAnother)
{
    std::vector<unsigned char> buffer{ 0x0, 0x1, 0x2 };
    buffer_view sut(buffer.data(), buffer.size());

    ASSERT_EQ(sut.cend() - sut.cbegin(), 3);
}

TEST(BufferViewConstIterator, SubtractNumberFromIterator)
{
    std::vector<unsigned char> buffer{ 0x0, 0x1, 0x2 };
    buffer_view sut(buffer.data(), buffer.size());

    ASSERT_EQ(sut.cend() - 3, sut.cbegin());
}

TEST(BufferViewConstIterator, SummarizeAnIteratorWithANumber)
{
    std::vector<unsigned char> buffer{ 0x0, 0x1, 0x2 };
    buffer_view sut(buffer.data(), buffer.size());

    ASSERT_EQ(sut.cbegin() + 3, sut.cend());
    ASSERT_EQ(3 + sut.cbegin(), sut.cend());
}

TEST(BufferViewReverseIterator, HasIteratorTraits)
{
    bool is_difference_type_same = std::is_same_v<buffer_view::reverse_iterator::difference_type, std::ptrdiff_t>;
    bool is_value_type_type = std::is_same_v<buffer_view::reverse_iterator::value_type, unsigned char>;
    bool is_pointer_type_same = std::is_same_v<buffer_view::reverse_iterator::pointer, unsigned char *>;
    bool is_reference_type_same = std::is_same_v<buffer_view::reverse_iterator::reference, unsigned char &>;
    bool is_iterator_category_same = std::is_same_v<buffer_view::reverse_iterator::iterator_category,
        std::random_access_iterator_tag>;

    ASSERT_TRUE(is_difference_type_same);
    ASSERT_TRUE(is_value_type_type);
    ASSERT_TRUE(is_pointer_type_same);
    ASSERT_TRUE(is_reference_type_same);
    ASSERT_TRUE(is_iterator_category_same);
}

TEST(BufferViewReverseIterator, IncrementsIteratorWithPrefixIncrementation)
{
    std::vector<unsigned char> buffer{ 0x1, 0x2 };
    buffer_view sut(buffer.data(), buffer.size());

    auto iter = sut.rbegin();
    ASSERT_EQ(*++iter, 0x1);
}

TEST(BufferViewReverseIterator, IncrementsIteratorWithPostfixIncrementation)
{
    std::vector<unsigned char> buffer{ 0x1, 0x2 };
    buffer_view sut(buffer.data(), buffer.size());

    auto iter = sut.rbegin();
    auto prev_iter = iter++;

    ASSERT_EQ(*prev_iter, 0x2);
    ASSERT_EQ(*iter, 0x1);
}

TEST(BufferViewReverseIterator, DecrementsIteratorWithPrefixDecrementation)
{
    std::vector<unsigned char> buffer{ 0x1, 0x2 };
    buffer_view sut(buffer.data(), buffer.size());

    auto iter = ++sut.rbegin();
    ASSERT_EQ(*--iter, 0x2);
}

TEST(BufferViewReverseIterator, DecrementsIteratorWithPostfixDecrementation)
{
    std::vector<unsigned char> buffer{ 0x1, 0x2 };
    buffer_view sut(buffer.data(), buffer.size());

    auto iter = ++sut.rbegin();
    auto prev_iter = iter--;

    ASSERT_EQ(*prev_iter, 0x1);
    ASSERT_EQ(*iter, 0x2);
}

TEST(BufferViewReverseIterator, AllowsDereferencing)
{
    std::vector<unsigned char> buffer{ 0x0 };
    buffer_view sut(buffer.data(), buffer.size());
    *(sut.data()) = 0x1;

    ASSERT_EQ(*sut.rbegin(), 0x1);
}

TEST(BufferViewReverseIterator, DetermineEqualIterators)
{
    std::vector<unsigned char> buffer{ 0x0, 0x1, 0x3 };
    buffer_view sut(buffer.data(), buffer.size());

    ASSERT_TRUE(sut.rbegin() == sut.rbegin());
}

TEST(BufferViewReverseIterator, DetermineNonEqualIterators)
{
    std::vector<unsigned char> buffer{ 0x0, 0x1, 0x3 };
    buffer_view sut(buffer.data(), buffer.size());

    ASSERT_TRUE(sut.rbegin() != sut.rend());
}

TEST(BufferViewReverseIterator, SubtractOneIteratorFromAnother)
{
    std::vector<unsigned char> buffer{ 0x0, 0x1, 0x3 };
    buffer_view sut(buffer.data(), buffer.size());

    ASSERT_EQ(sut.rend() - sut.rbegin(), 3);
}

TEST(BufferViewReverseIterator, SubtractNumberFromIterator)
{
    std::vector<unsigned char> buffer{ 0x0, 0x1, 0x3 };
    buffer_view sut(buffer.data(), buffer.size());

    ASSERT_EQ(sut.rend() - 3, sut.rbegin());
}

TEST(BufferViewReverseIterator, SummarizeAnIteratorWithANumber)
{
    std::vector<unsigned char> buffer{ 0x0, 0x1, 0x3 };
    buffer_view sut(buffer.data(), buffer.size());

    ASSERT_EQ(sut.rbegin() + 3, sut.rend());
    ASSERT_EQ(3 + sut.rbegin(), sut.rend());
}

TEST(BufferViewReverseIterator, ConvertsToIterator)
{
    std::vector<unsigned char> buffer{ 0x0 };
    buffer_view sut(buffer.data(), buffer.size());

    ASSERT_EQ(sut.begin() + 1, sut.rbegin().base());
}

TEST(BufferViewConstReverseIterator, HasIteratorTraits)
{
    bool is_difference_type_same = std::is_same_v<buffer_view::const_reverse_iterator::difference_type, std::ptrdiff_t>;
    bool is_value_type_type = std::is_same_v<buffer_view::const_reverse_iterator::value_type, unsigned char>;
    bool is_pointer_type_same = std::is_same_v<buffer_view::const_reverse_iterator::pointer, const unsigned char *>;
    bool is_reference_type_same = std::is_same_v<buffer_view::const_reverse_iterator::reference, const unsigned char &>;
    bool is_iterator_category_same = std::is_same_v<buffer_view::const_reverse_iterator::iterator_category,
                                                    std::random_access_iterator_tag>;

    ASSERT_TRUE(is_difference_type_same);
    ASSERT_TRUE(is_value_type_type);
    ASSERT_TRUE(is_pointer_type_same);
    ASSERT_TRUE(is_reference_type_same);
    ASSERT_TRUE(is_iterator_category_same);
}

TEST(BufferViewConstReverseIterator, IncrementsIteratorWithPrefixIncrementation)
{
    std::vector<unsigned char> buffer{ 0x1, 0x2 };
    buffer_view sut(buffer.data(), buffer.size());

    auto iter = sut.crbegin();
    ASSERT_EQ(*++iter, 0x1);
}

TEST(BufferViewConstReverseIterator, IncrementsIteratorWithPostfixIncrementation)
{
    std::vector<unsigned char> buffer{ 0x1, 0x2 };
    buffer_view sut(buffer.data(), buffer.size());

    auto iter = sut.crbegin();
    auto prev_iter = iter++;

    ASSERT_EQ(*prev_iter, 0x2);
    ASSERT_EQ(*iter, 0x1);
}

TEST(BufferViewConstReverseIterator, DecrementsIteratorWithPrefixDecrementation)
{
    std::vector<unsigned char> buffer{ 0x1, 0x2 };
    buffer_view sut(buffer.data(), buffer.size());

    auto iter = ++sut.crbegin();
    ASSERT_EQ(*--iter, 0x2);
}

TEST(BufferViewConstReverseIterator, DecrementsIteratorWithPostfixDecrementation)
{
    std::vector<unsigned char> buffer{ 0x1, 0x2 };
    buffer_view sut(buffer.data(), buffer.size());

    auto iter = ++sut.crbegin();
    auto prev_iter = iter--;

    ASSERT_EQ(*prev_iter, 0x1);
    ASSERT_EQ(*iter, 0x2);
}

TEST(BufferViewConstReverseIterator, AllowsDereferencing)
{
    std::vector<unsigned char> buffer{ 0x1, 0x2 };
    buffer_view sut(buffer.data(), buffer.size());

    ASSERT_EQ(*sut.crbegin(), 0x2);
}

TEST(BufferViewConstReverseIterator, DetermineEqualIterators)
{
    std::vector<unsigned char> buffer{ 0x1, 0x2, 0x3 };
    buffer_view sut(buffer.data(), buffer.size());

    ASSERT_TRUE(sut.crbegin() == sut.crbegin());
}

TEST(BufferViewConstReverseIterator, DetermineNonEqualIterators)
{
    std::vector<unsigned char> buffer{ 0x1, 0x2, 0x3 };
    buffer_view sut(buffer.data(), buffer.size());

    ASSERT_TRUE(sut.crbegin() != sut.crend());
}

TEST(BufferViewConstReverseIterator, SubtractOneIteratorFromAnother)
{
    std::vector<unsigned char> buffer{ 0x1, 0x2, 0x3 };
    buffer_view sut(buffer.data(), buffer.size());

    ASSERT_EQ(sut.crend() - sut.crbegin(), 3);
}

TEST(BufferViewConstReverseIterator, SubtractNumberFromIterator)
{
    std::vector<unsigned char> buffer{ 0x1, 0x2, 0x3 };
    buffer_view sut(buffer.data(), buffer.size());

    ASSERT_EQ(sut.crend() - 3, sut.crbegin());
}

TEST(BufferViewConstReverseIterator, SummarizeAnIteratorWithANumber)
{
    std::vector<unsigned char> buffer{ 0x1, 0x2, 0x3 };
    buffer_view sut(buffer.data(), buffer.size());

    ASSERT_EQ(sut.crbegin() + 3, sut.crend());
    ASSERT_EQ(3 + sut.crbegin(), sut.crend());
}

TEST(BufferViewConstReverseIterator, ConvertsToIterator)
{
    std::vector<unsigned char> buffer{ 0x1 };
    buffer_view sut(buffer.data(), buffer.size());

    ASSERT_EQ(sut.cbegin() + 1, sut.crbegin().base());
}
