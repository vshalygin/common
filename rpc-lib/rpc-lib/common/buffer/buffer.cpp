#include "buffer.h"

#include <utility>

namespace vsh::rpc {
    buffer::buffer()
        : buffer_(nullptr)
        , capacity_(0)
    {}

    buffer::buffer(size_t capacity)
        : buffer_(new char[capacity])
        , capacity_(capacity)
    {}

    buffer::~buffer()
    {
        delete[] buffer_;
    }

    buffer::buffer(buffer &&other)
        : buffer()
    {
        std::swap(buffer_, other.buffer_);
        std::swap(capacity_, other.capacity_);
    }

    buffer &buffer::operator=(buffer &&other)
    {
        std::swap(buffer_, other.buffer_);
        std::swap(capacity_, other.capacity_);

        return *this;
    }

    void buffer::reserve(size_t new_cap)
    {
        if(capacity_ >= new_cap) {
            return;
        }

        char *new_buf = new char[new_cap];

        delete[] buffer_;
        buffer_ = new_buf;
        capacity_ = new_cap;
    }

    char *buffer::data()
    {
        return buffer_;
    }

    const char *buffer::data() const
    {
        return buffer_;
    }

    size_t buffer::capacity() const
    {
        return capacity_;
    }
}
