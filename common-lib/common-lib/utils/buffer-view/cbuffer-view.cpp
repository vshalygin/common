#include "cbuffer-view.h"

#include <stdexcept>

namespace vsh::cl {
    cbuffer_view::iterator::iterator(const unsigned char *buffer) noexcept
        : buffer_(buffer)
    {}

    cbuffer_view::iterator &cbuffer_view::iterator::operator++() noexcept
    {
        ++buffer_;
        return *this;
    }

    cbuffer_view::iterator cbuffer_view::iterator::operator++(int) noexcept
    {
        auto prev = buffer_++;
        return iterator(prev);
    }

    cbuffer_view::iterator &cbuffer_view::iterator::operator--() noexcept
    {
        --buffer_;
        return *this;
    }

    cbuffer_view::iterator cbuffer_view::iterator::operator--(int) noexcept
    {
        auto prev = buffer_--;
        return iterator(prev);
    }

    const unsigned char &cbuffer_view::iterator::operator*() const noexcept
    {
        return *buffer_;
    }

    bool cbuffer_view::iterator::operator==(const iterator &other) const noexcept
    {
        return buffer_ == other.buffer_;
    }

    bool cbuffer_view::iterator::operator!=(const iterator &other) const noexcept
    {
        return !(other == *this);
    }

    cbuffer_view::iterator cbuffer_view::iterator::operator+(difference_type offset) const noexcept
    {
        return iterator(buffer_ + offset);
    }

    cbuffer_view::iterator cbuffer_view::iterator::operator-(difference_type offset) const noexcept
    {
        return iterator(buffer_ - offset);
    }

    cbuffer_view::iterator::difference_type cbuffer_view::iterator::operator-(iterator other) const noexcept
    {
        return buffer_ - other.buffer_;
    }

    cbuffer_view::cbuffer_view() noexcept
        : cbuffer_view(nullptr, 0)
    {}

    cbuffer_view::cbuffer_view(const unsigned char *buffer, size_t size) noexcept
        : buffer_(buffer)
        , size_(size)
    {}

    cbuffer_view::cbuffer_view(const buffer &buf) noexcept
        : cbuffer_view(buf.data(), buf.size())
    {}

    const unsigned char *cbuffer_view::data() const noexcept
    {
        return buffer_;
    }

    size_t cbuffer_view::size() const noexcept
    {
        return size_;
    }

    const unsigned char &cbuffer_view::operator[](size_t pos) const noexcept
    {
        return *(buffer_ + pos);
    }

    cbuffer_view::operator bool() const noexcept
    {
        return buffer_;
    }


    const unsigned char &cbuffer_view::at(size_t pos) const
    {
        if(pos >= size_) {
            throw std::out_of_range("attempt to access an element out of the buffer");
        }

        return (*this)[pos];
    }

    cbuffer_view::iterator cbuffer_view::begin() const noexcept
    {
        return iterator(buffer_);
    }

    cbuffer_view::iterator cbuffer_view::end() const noexcept
    {
        return iterator(buffer_ + size_);
    }

    cbuffer_view::const_iterator cbuffer_view::cbegin() const noexcept
    {
        return const_iterator(buffer_);
    }

    cbuffer_view::const_iterator cbuffer_view::cend() const noexcept
    {
        return const_iterator(buffer_ + size_);
    }

    cbuffer_view::reverse_iterator cbuffer_view::rbegin() const noexcept
    {
        return reverse_iterator(buffer_ + size_);
    }

    cbuffer_view::reverse_iterator cbuffer_view::rend() const noexcept
    {
        return reverse_iterator(buffer_);
    }

    cbuffer_view::const_reverse_iterator cbuffer_view::crbegin() const noexcept
    {
        return const_reverse_iterator(buffer_ + size_);
    }

    cbuffer_view::const_reverse_iterator cbuffer_view::crend() const noexcept
    {
        return const_reverse_iterator(buffer_);
    }

    cbuffer_view::iterator operator+(cbuffer_view::iterator::difference_type offset,
                                     const cbuffer_view::iterator &rhs) noexcept
    {
        return rhs + offset;
    }

    cbuffer_view::reverse_iterator operator+(cbuffer_view::reverse_iterator::difference_type offset,
                                             const cbuffer_view::reverse_iterator &rhs) noexcept
    {
        return rhs + offset;
    }
}
