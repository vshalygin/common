#include "buffer.h"

#include <utility>
#include <stdexcept>

namespace vsh::cl {
    buffer::iterator::iterator(unsigned char *buffer) noexcept
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

    unsigned char &buffer::iterator::operator*() noexcept
    {
        return const_cast<unsigned char &>(*static_cast<const iterator &>(*this));
    }

    const unsigned char &buffer::iterator::operator*() const noexcept
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

    buffer::const_iterator::const_iterator(const unsigned char *buffer) noexcept
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

    const unsigned char &buffer::const_iterator::operator*() const noexcept
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
        : m_buffer(new unsigned char[size])
        , m_size(size)
    {}

    buffer::~buffer()
    {
        delete[] m_buffer;
    }

    buffer::buffer(const buffer &other)
        : buffer()
    {
        m_buffer = new unsigned char[other.m_size];
        m_size = other.m_size;

        for(size_t i = 0; i < m_size; ++i) {
            m_buffer[i] = other[i];
        }
    }

    buffer &buffer::operator=(const buffer &other)
    {
        if(this == &other) {
            return *this;
        }

        auto new_buf = new unsigned char[other.m_size];
        for(size_t i = 0; i < other.m_size; ++i) {
            new_buf[i] = other[i];
        }

        delete[] m_buffer;
        m_buffer = new_buf;
        m_size = other.m_size;

        return *this;
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

    unsigned char *buffer::data() noexcept
    {
        return const_cast<unsigned char *>(static_cast<const buffer &>(*this).data());
    }

    const unsigned char *buffer::data() const noexcept
    {
        return m_buffer;
    }

    size_t buffer::size() const noexcept
    {
        return m_size;
    }

    unsigned char &buffer::operator[](size_t pos) noexcept
    {
        return const_cast<unsigned char &>(static_cast<const buffer &>(*this)[pos]);
    }

    const unsigned char &buffer::operator[](size_t pos) const noexcept
    {
        return *(m_buffer + pos);
    }

    buffer::operator bool() const noexcept
    {
        return m_buffer;
    }

    unsigned char &buffer::at(size_t pos)
    {
        return const_cast<unsigned char &>(static_cast<const buffer &>(*this).at(pos));
    }

    const unsigned char &buffer::at(size_t pos) const
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
