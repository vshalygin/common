#include "buffer-view.h"

#include <stdexcept>

namespace vsh::cl {
    buffer_view::iterator::iterator(unsigned char *buffer) noexcept
        : m_buffer(buffer)
    {}

    buffer_view::iterator &buffer_view::iterator::operator++() noexcept
    {
        ++m_buffer;
        return *this;
    }

    buffer_view::iterator buffer_view::iterator::operator++(int) noexcept
    {
        auto prev = m_buffer++;
        return iterator(prev);
    }

    buffer_view::iterator &buffer_view::iterator::operator--() noexcept
    {
        --m_buffer;
        return *this;
    }

    buffer_view::iterator buffer_view::iterator::operator--(int) noexcept
    {
        auto prev = m_buffer--;
        return iterator(prev);
    }

    unsigned char &buffer_view::iterator::operator*() noexcept
    {
        return const_cast<unsigned char &>(*static_cast<const iterator &>(*this));
    }

    const unsigned char &buffer_view::iterator::operator*() const noexcept
    {
        return *m_buffer;
    }

    bool buffer_view::iterator::operator==(const iterator &other) const noexcept
    {
        return m_buffer == other.m_buffer;
    }

    bool buffer_view::iterator::operator!=(const iterator &other) const noexcept
    {
        return !(other == *this);
    }

    buffer_view::iterator buffer_view::iterator::operator+(difference_type offset) const noexcept
    {
        return iterator(m_buffer + offset);
    }

    buffer_view::iterator buffer_view::iterator::operator-(difference_type offset) const noexcept
    {
        return iterator(m_buffer - offset);
    }

    buffer_view::iterator::difference_type buffer_view::iterator::operator-(iterator other) const noexcept
    {
        return m_buffer - other.m_buffer;
    }

    buffer_view::const_iterator::const_iterator(const unsigned char *buffer) noexcept
        : m_buffer(buffer)
    {}

    buffer_view::const_iterator &buffer_view::const_iterator::operator++() noexcept
    {
        ++m_buffer;
        return *this;
    }

    buffer_view::const_iterator buffer_view::const_iterator::operator++(int) noexcept
    {
        auto prev = m_buffer++;
        return const_iterator(prev);
    }

    buffer_view::const_iterator &buffer_view::const_iterator::operator--() noexcept
    {
        --m_buffer;
        return *this;
    }

    buffer_view::const_iterator buffer_view::const_iterator::operator--(int) noexcept
    {
        auto prev = m_buffer--;
        return const_iterator(prev);
    }

    const unsigned char &buffer_view::const_iterator::operator*() const noexcept
    {
        return *m_buffer;
    }

    bool buffer_view::const_iterator::operator==(const const_iterator &other) const noexcept
    {
        return m_buffer == other.m_buffer;
    }

    bool buffer_view::const_iterator::operator!=(const const_iterator &other) const noexcept
    {
        return !(other == *this);
    }

    buffer_view::const_iterator buffer_view::const_iterator::operator+(difference_type offset) const noexcept
    {
        return const_iterator(m_buffer + offset);
    }

    buffer_view::const_iterator buffer_view::const_iterator::operator-(difference_type offset) const noexcept
    {
        return const_iterator(m_buffer - offset);
    }

    buffer_view::const_iterator::difference_type buffer_view::const_iterator::operator-(const_iterator other) const noexcept
    {
        return m_buffer - other.m_buffer;
    }

    buffer_view::buffer_view() noexcept
        : buffer_view(nullptr, 0)
    {}

    buffer_view::buffer_view(unsigned char *buffer, size_t size) noexcept
        : m_buffer(buffer)
        , m_size(size)
    {}

    buffer_view::buffer_view(buffer &buf) noexcept
        : buffer_view(buf.data(), buf.size())
    {}

    unsigned char *buffer_view::data() noexcept
    {
        return const_cast<unsigned char *>(static_cast<const buffer_view &>(*this).data());
    }

    const unsigned char *buffer_view::data() const noexcept
    {
        return m_buffer;
    }

    size_t buffer_view::size() const noexcept
    {
        return m_size;
    }

    const unsigned char &buffer_view::operator[](size_t pos) const noexcept
    {
        return *(m_buffer + pos);
    }

    unsigned char &buffer_view::operator[](size_t pos) noexcept
    {
        return const_cast<unsigned char &>(static_cast<const buffer_view &>(*this)[pos]);
    }

    buffer_view::operator bool() const noexcept
    {
        return m_buffer;
    }

    unsigned char &buffer_view::at(size_t pos)
    {
        return const_cast<unsigned char &>(static_cast<const buffer_view &>(*this).at(pos));
    }

    const unsigned char &buffer_view::at(size_t pos) const
    {
        if(pos >= m_size) {
            throw std::out_of_range("attempt to access an element out of the buffer");
        }

        return (*this)[pos];
    }

    buffer_view::iterator buffer_view::begin() noexcept
    {
        return iterator(m_buffer);
    }

    buffer_view::iterator buffer_view::end() noexcept
    {
        return iterator(m_buffer + m_size);
    }

    buffer_view::const_iterator buffer_view::cbegin() const noexcept
    {
        return const_iterator(m_buffer);
    }

    buffer_view::const_iterator buffer_view::cend() const noexcept
    {
        return const_iterator(m_buffer + m_size);
    }

    buffer_view::reverse_iterator buffer_view::rbegin() noexcept
    {
        return reverse_iterator(m_buffer + m_size);
    }

    buffer_view::reverse_iterator buffer_view::rend() noexcept
    {
        return reverse_iterator(m_buffer);
    }

    buffer_view::const_reverse_iterator buffer_view::crbegin() const noexcept
    {
        return const_reverse_iterator(m_buffer + m_size);
    }

    buffer_view::const_reverse_iterator buffer_view::crend() const noexcept
    {
        return const_reverse_iterator(m_buffer);
    }

    buffer_view::iterator operator+(buffer_view::iterator::difference_type offset,
                                    const buffer_view::iterator &rhs) noexcept
    {
        return rhs + offset;
    }

    buffer_view::const_iterator operator+(buffer_view::const_iterator::difference_type offset,
                                          const buffer_view::const_iterator &rhs) noexcept
    {
        return rhs + offset;
    }

    buffer_view::reverse_iterator operator+(buffer_view::reverse_iterator::difference_type offset,
                                            const buffer_view::reverse_iterator &rhs) noexcept
    {
        return rhs + offset;
    }

    buffer_view::const_reverse_iterator operator+(buffer_view::const_reverse_iterator::difference_type offset,
                                                  const buffer_view::const_reverse_iterator &rhs) noexcept
    {
        return rhs + offset;
    }
}
