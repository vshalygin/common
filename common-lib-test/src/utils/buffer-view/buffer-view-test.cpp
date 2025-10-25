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
