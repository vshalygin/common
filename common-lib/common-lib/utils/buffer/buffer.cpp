#include "buffer.h"

#include <utility>
#include <stdexcept>

namespace vsh::common_lib {
    buffer::iterator::iterator(unsigned char *buffer) noexcept
        : buffer_(buffer)
    {}

    buffer::iterator &buffer::iterator::operator++() noexcept
    {
        ++buffer_;
        return *this;
    }

    buffer::iterator buffer::iterator::operator++(int) noexcept
    {
        auto prev = buffer_++;
        return buffer::iterator(prev);
    }

    buffer::iterator &buffer::iterator::operator--() noexcept
    {
        --buffer_;
        return *this;
    }

    buffer::iterator buffer::iterator::operator--(int) noexcept
    {
        auto prev = buffer_--;
        return buffer::iterator(prev);
    }

    unsigned char &buffer::iterator::operator*() noexcept
    {
        return const_cast<unsigned char &>(*static_cast<const iterator &>(*this));
    }

    const unsigned char &buffer::iterator::operator*() const noexcept
    {
        return *buffer_;
    }

    bool buffer::iterator::operator==(const iterator &other) const noexcept
    {
        return buffer_ == other.buffer_;
    }

    bool buffer::iterator::operator!=(const iterator &other) const noexcept
    {
        return !(other == *this);
    }

    buffer::iterator buffer::iterator::operator+(difference_type offset) const noexcept
    {
        return iterator(buffer_ + offset);
    }

    buffer::iterator buffer::iterator::operator-(difference_type offset) const noexcept
    {
        return iterator(buffer_ - offset);
    }

    buffer::iterator::difference_type buffer::iterator::operator-(iterator other) const noexcept
    {
        return buffer_ - other.buffer_;
    }

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

    buffer::operator bool() const noexcept
    {
        return buffer_;
    }

    unsigned char &buffer::at(size_t pos)
    {
        return const_cast<unsigned char &>(static_cast<const buffer &>(*this).at(pos));
    }

    const unsigned char &buffer::at(size_t pos) const
    {
        if(pos >= size_) {
            throw std::out_of_range("attempt to access an element out of the buffer");
        }

        return (*this)[pos];
    }

    buffer::iterator buffer::begin() noexcept
    {
        return iterator(buffer_);
    }

    buffer::iterator buffer::end() noexcept
    {
        return iterator(buffer_ + size_);
    }

    buffer::iterator operator+(buffer::iterator::difference_type offset,
                               const buffer::iterator &rhs) noexcept
    {
        return rhs + offset;
    }
}
