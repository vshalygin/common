#include "buffer.h"

#include <utility>

namespace vsh::common_lib {
    buffer::buffer() noexcept
        : buffer_(nullptr)
        , size_(0)
    {}

    buffer::buffer(size_t size)
        : buffer_(new unsigned char[size])
        , size_(size)
    {}

    buffer::~buffer()
    {
        delete[] buffer_;
    }

    buffer::buffer(buffer &&other) noexcept
        : buffer()
    {
        std::swap(buffer_, other.buffer_);
        std::swap(size_, other.size_);
    }

    buffer &buffer::operator=(buffer &&other) noexcept
    {
        std::swap(buffer_, other.buffer_);
        std::swap(size_, other.size_);

        return *this;
    }

    unsigned char *buffer::data() noexcept
    {
        return const_cast<unsigned char *>(static_cast<const buffer &>(*this).data());
    }

    const unsigned char *buffer::data() const noexcept
    {
        return buffer_;
    }

    size_t buffer::size() const noexcept
    {
        return size_;
    }

    unsigned char &buffer::operator[](size_t pos) noexcept
    {
        return const_cast<unsigned char &>(static_cast<const buffer &>(*this)[pos]);
    }

    const unsigned char &buffer::operator[](size_t pos) const noexcept
    {
        return *(buffer_ + pos);
    }
}
