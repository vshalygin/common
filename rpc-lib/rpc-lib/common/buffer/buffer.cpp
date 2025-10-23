#include "buffer.h"

#include <utility>

namespace vsh::rpc {
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

    void buffer::reallocate(size_t new_size)
    {
        unsigned char *new_buf = new unsigned char[new_size];

        delete[] buffer_;
        buffer_ = new_buf;
        size_ = new_size;
    }

    unsigned char *buffer::data()
    {
        return buffer_;
    }

    const unsigned char *buffer::data() const
    {
        return buffer_;
    }

    size_t buffer::size() const
    {
        return size_;
    }
}
