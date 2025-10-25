#include <common-lib/utils/buffer-view/cbuffer-view.h>

#include <gtest/gtest.h>

using namespace vsh::common_lib;
using namespace testing;

TEST(CBufferView, ReturnsStoringData)
{
    std::vector<unsigned char> buffer{ 0x34, 0x22 };

    cbuffer_view sut(buffer.data(), buffer.size());

    ASSERT_EQ(sut.data(), buffer.data());
}

TEST(CBufferView, ReturnsSizeOfStoringBuffer)
{
    std::vector<unsigned char> buffer{ 0x34, 0x22 };

    const cbuffer_view sut(buffer.data(), buffer.size());

    ASSERT_EQ(sut.size(), buffer.size());
}
