#include "buffer.h"

#include <utility>
#include <stdexcept>

namespace vsh::cl {
    buffer::iterator::iterator(std::byte *buffer) noexcept
        : m_buffer(buffer)
    {}

    buffer::iterator &buffer::iterator::operator++() noexcept
    {
        ++m_buffer;
        return *this;
    }

    buffer::iterator buffer::iterator::operator++(int) noexcept
    {
        auto prev = m_buffer++;
        return buffer::iterator(prev);
    }

    buffer::iterator &buffer::iterator::operator--() noexcept
    {
        --m_buffer;
        return *this;
    }

    buffer::iterator buffer::iterator::operator--(int) noexcept
    {
        auto prev = m_buffer--;
        return iterator(prev);
    }

    std::byte &buffer::iterator::operator*() noexcept
    {
        return const_cast<std::byte &>(*static_cast<const iterator &>(*this));
    }

    const std::byte &buffer::iterator::operator*() const noexcept
    {
        return *m_buffer;
    }

    bool buffer::iterator::operator==(const iterator &other) const noexcept
    {
        return m_buffer == other.m_buffer;
    }

    bool buffer::iterator::operator!=(const iterator &other) const noexcept
    {
        return !(other == *this);
    }

    buffer::iterator buffer::iterator::operator+(difference_type offset) const noexcept
    {
        return iterator(m_buffer + offset);
    }

    buffer::iterator buffer::iterator::operator-(difference_type offset) const noexcept
    {
        return iterator(m_buffer - offset);
    }

    buffer::iterator::difference_type buffer::iterator::operator-(iterator other) const noexcept
    {
        return m_buffer - other.m_buffer;
    }

    buffer::const_iterator::const_iterator(const std::byte *buffer) noexcept
        : m_buffer(buffer)
    {}

    buffer::const_iterator &buffer::const_iterator::operator++() noexcept
    {
        ++m_buffer;
        return *this;
    }

    buffer::const_iterator buffer::const_iterator::operator++(int) noexcept
    {
        auto prev = m_buffer++;
        return const_iterator(prev);
    }

    buffer::const_iterator &buffer::const_iterator::operator--() noexcept
    {
        --m_buffer;
        return *this;
    }

    buffer::const_iterator buffer::const_iterator::operator--(int) noexcept
    {
        auto prev = m_buffer--;
        return const_iterator(prev);
    }

    const std::byte &buffer::const_iterator::operator*() const noexcept
    {
        return *m_buffer;
    }

    bool buffer::const_iterator::operator==(const const_iterator &other) const noexcept
    {
        return m_buffer == other.m_buffer;
    }

    bool buffer::const_iterator::operator!=(const const_iterator &other) const noexcept
    {
        return !(other == *this);
    }

    buffer::const_iterator buffer::const_iterator::operator+(difference_type offset) const noexcept
    {
        return const_iterator(m_buffer + offset);
    }

    buffer::const_iterator buffer::const_iterator::operator-(difference_type offset) const noexcept
    {
        return const_iterator(m_buffer - offset);
    }

    buffer::const_iterator::difference_type buffer::const_iterator::operator-(const_iterator other) const noexcept
    {
        return m_buffer - other.m_buffer;
    }

    buffer::buffer() noexcept
        : m_buffer(nullptr)
        , m_size(0)
    {}

    buffer::buffer(size_t size)
        : m_buffer(new std::byte[size])
        , m_size(size)
    {}

    buffer::~buffer()
    {
        delete[] m_buffer;
    }

    buffer::buffer(buffer &&other) noexcept
        : buffer()
    {
        std::swap(m_buffer, other.m_buffer);
        std::swap(m_size, other.m_size);
    }

    buffer &buffer::operator=(buffer &&other) noexcept
    {
        std::swap(m_buffer, other.m_buffer);
        std::swap(m_size, other.m_size);

        return *this;
    }

    buffer buffer::copy() const
    {
        buffer buf;
        buf.m_buffer = new std::byte[m_size];
        buf.m_size = m_size;
        for(size_t i = 0; i < buf.m_size; ++i) {
            buf.m_buffer[i] = m_buffer[i];
        }

        return buf;
    }

    std::byte *buffer::data() noexcept
    {
        return const_cast<std::byte *>(static_cast<const buffer &>(*this).data());
    }

    const std::byte *buffer::data() const noexcept
    {
        return m_buffer;
    }

    size_t buffer::size() const noexcept
    {
        return m_size;
    }

    std::byte &buffer::operator[](size_t pos) noexcept
    {
        return const_cast<std::byte &>(static_cast<const buffer &>(*this)[pos]);
    }

    const std::byte &buffer::operator[](size_t pos) const noexcept
    {
        return *(m_buffer + pos);
    }

    buffer::operator bool() const noexcept
    {
        return m_buffer;
    }

    std::byte &buffer::at(size_t pos)
    {
        return const_cast<std::byte &>(static_cast<const buffer &>(*this).at(pos));
    }

    const std::byte &buffer::at(size_t pos) const
    {
        if(pos >= m_size) {
            throw std::out_of_range("attempt to access an element out of the buffer");
        }

        return (*this)[pos];
    }

    buffer::iterator buffer::begin() noexcept
    {
        return iterator(m_buffer);
    }

    buffer::iterator buffer::end() noexcept
    {
        return iterator(m_buffer + m_size);
    }

    buffer::const_iterator buffer::cbegin() const noexcept
    {
        return const_iterator(m_buffer);
    }

    buffer::const_iterator buffer::cend() const noexcept
    {
        return const_iterator(m_buffer + m_size);
    }

    buffer::reverse_iterator buffer::rbegin() noexcept
    {
        return reverse_iterator(m_buffer + m_size);
    }

    buffer::reverse_iterator buffer::rend() noexcept
    {
        return reverse_iterator(m_buffer);
    }

    buffer::const_reverse_iterator buffer::crbegin() const noexcept
    {
        return const_reverse_iterator(m_buffer + m_size);
    }

    buffer::const_reverse_iterator buffer::crend() const noexcept
    {
        return const_reverse_iterator(m_buffer);
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
