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
        return iterator(prev);
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

    buffer::const_iterator::const_iterator(const unsigned char *buffer) noexcept
        : buffer_(buffer)
    {}

    buffer::const_iterator &buffer::const_iterator::operator++() noexcept
    {
        ++buffer_;
        return *this;
    }

    buffer::const_iterator buffer::const_iterator::operator++(int) noexcept
    {
        auto prev = buffer_++;
        return const_iterator(prev);
    }

    buffer::const_iterator &buffer::const_iterator::operator--() noexcept
    {
        --buffer_;
        return *this;
    }

    buffer::const_iterator buffer::const_iterator::operator--(int) noexcept
    {
        auto prev = buffer_--;
        return const_iterator(prev);
    }

    const unsigned char &buffer::const_iterator::operator*() const noexcept
    {
        return *buffer_;
    }

    bool buffer::const_iterator::operator==(const const_iterator &other) const noexcept
    {
        return buffer_ == other.buffer_;
    }

    bool buffer::const_iterator::operator!=(const const_iterator &other) const noexcept
    {
        return !(other == *this);
    }

    buffer::const_iterator buffer::const_iterator::operator+(difference_type offset) const noexcept
    {
        return const_iterator(buffer_ + offset);
    }

    buffer::const_iterator buffer::const_iterator::operator-(difference_type offset) const noexcept
    {
        return const_iterator(buffer_ - offset);
    }

    buffer::const_iterator::difference_type buffer::const_iterator::operator-(const_iterator other) const noexcept
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

    buffer::buffer(const buffer &other)
        : buffer()
    {
        buffer_ = new unsigned char[other.size_];
        size_ = other.size_;

        for(size_t i = 0; i < size_; ++i) {
            buffer_[i] = other[i];
        }
    }

    buffer &buffer::operator=(const buffer &other)
    {
        if(this == &other) {
            return *this;
        }

        auto new_buf = new unsigned char[other.size_];
        for(size_t i = 0; i < other.size_; ++i) {
            new_buf[i] = other[i];
        }

        delete[] buffer_;
        buffer_ = new_buf;
        size_ = other.size_;

        return *this;
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

    buffer::const_iterator buffer::cbegin() const noexcept
    {
        return const_iterator(buffer_);
    }

    buffer::const_iterator buffer::cend() const noexcept
    {
        return const_iterator(buffer_ + size_);
    }

    buffer::reverse_iterator buffer::rbegin() noexcept
    {
        return reverse_iterator(buffer_ + size_);
    }

    buffer::reverse_iterator buffer::rend() noexcept
    {
        return reverse_iterator(buffer_);
    }

    buffer::const_reverse_iterator buffer::crbegin() const noexcept
    {
        return const_reverse_iterator(buffer_ + size_);
    }

    buffer::const_reverse_iterator buffer::crend() const noexcept
    {
        return const_reverse_iterator(buffer_);
    }

    buffer::iterator operator+(buffer::iterator::difference_type offset,
                               const buffer::iterator &rhs) noexcept
    {
        return rhs + offset;
    }

    buffer::const_iterator operator+(buffer::const_iterator::difference_type offset,
                                     const buffer::const_iterator &rhs) noexcept
    {
        return rhs + offset;
    }

    buffer::reverse_iterator operator+(buffer::reverse_iterator::difference_type offset,
                                       const buffer::reverse_iterator &rhs) noexcept
    {
        return rhs + offset;
    }

    buffer::const_reverse_iterator operator+(buffer::const_reverse_iterator::difference_type offset,
                                             const buffer::const_reverse_iterator &rhs) noexcept
    {
        return rhs + offset;
    }
}
