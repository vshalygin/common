#include "cbuffer-view.h"

#include <stdexcept>

namespace vsh::cl {
    cbuffer_view::iterator::iterator(const std::byte *buffer) noexcept
        : m_buffer(buffer)
    {}

    cbuffer_view::iterator &cbuffer_view::iterator::operator++() noexcept
    {
        ++m_buffer;
        return *this;
    }

    cbuffer_view::iterator cbuffer_view::iterator::operator++(int) noexcept
    {
        auto prev = m_buffer++;
        return iterator(prev);
    }

    cbuffer_view::iterator &cbuffer_view::iterator::operator--() noexcept
    {
        --m_buffer;
        return *this;
    }

    cbuffer_view::iterator cbuffer_view::iterator::operator--(int) noexcept
    {
        auto prev = m_buffer--;
        return iterator(prev);
    }

    const std::byte &cbuffer_view::iterator::operator*() const noexcept
    {
        return *m_buffer;
    }

    bool cbuffer_view::iterator::operator==(const iterator &other) const noexcept
    {
        return m_buffer == other.m_buffer;
    }

    bool cbuffer_view::iterator::operator!=(const iterator &other) const noexcept
    {
        return !(other == *this);
    }

    cbuffer_view::iterator cbuffer_view::iterator::operator+(difference_type offset) const noexcept
    {
        return iterator(m_buffer + offset);
    }

    cbuffer_view::iterator cbuffer_view::iterator::operator-(difference_type offset) const noexcept
    {
        return iterator(m_buffer - offset);
    }

    cbuffer_view::iterator::difference_type cbuffer_view::iterator::operator-(iterator other) const noexcept
    {
        return m_buffer - other.m_buffer;
    }

    cbuffer_view::cbuffer_view() noexcept
        : cbuffer_view(nullptr, 0)
    {}

    cbuffer_view::cbuffer_view(const std::byte *buffer, size_t size) noexcept
        : m_buffer(buffer)
        , m_size(size)
    {}

    cbuffer_view::cbuffer_view(const buffer &buf) noexcept
        : cbuffer_view(buf.data(), buf.size())
    {}

    const std::byte *cbuffer_view::data() const noexcept
    {
        return m_buffer;
    }

    size_t cbuffer_view::size() const noexcept
    {
        return m_size;
    }

    const std::byte &cbuffer_view::operator[](size_t pos) const noexcept
    {
        return *(m_buffer + pos);
    }

    cbuffer_view::operator bool() const noexcept
    {
        return m_buffer;
    }


    const std::byte &cbuffer_view::at(size_t pos) const
    {
        if(pos >= m_size) {
            throw std::out_of_range("attempt to access an element out of the buffer");
        }

        return (*this)[pos];
    }

    cbuffer_view::iterator cbuffer_view::begin() const noexcept
    {
        return iterator(m_buffer);
    }

    cbuffer_view::iterator cbuffer_view::end() const noexcept
    {
        return iterator(m_buffer + m_size);
    }

    cbuffer_view::const_iterator cbuffer_view::cbegin() const noexcept
    {
        return const_iterator(m_buffer);
    }

    cbuffer_view::const_iterator cbuffer_view::cend() const noexcept
    {
        return const_iterator(m_buffer + m_size);
    }

    cbuffer_view::reverse_iterator cbuffer_view::rbegin() const noexcept
    {
        return reverse_iterator(m_buffer + m_size);
    }

    cbuffer_view::reverse_iterator cbuffer_view::rend() const noexcept
    {
        return reverse_iterator(m_buffer);
    }

    cbuffer_view::const_reverse_iterator cbuffer_view::crbegin() const noexcept
    {
        return const_reverse_iterator(m_buffer + m_size);
    }

    cbuffer_view::const_reverse_iterator cbuffer_view::crend() const noexcept
    {
        return const_reverse_iterator(m_buffer);
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

    bool operator==(const cbuffer_view &lhs, const cbuffer_view &rhs) noexcept
    {
        if(lhs.size() != rhs.size()) {
            return false;
        }
        for(size_t i = 0; i < lhs.size(); ++i) {
            if(lhs[i] != rhs[i]) {
                return false;
            }
        }
        return true;
    }

    bool operator!=(const cbuffer_view &lhs, const cbuffer_view &rhs) noexcept
    {
        return !(lhs == rhs);
    }
}
