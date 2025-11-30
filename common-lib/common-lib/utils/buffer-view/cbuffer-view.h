#pragma once
#include "common-lib/utils/buffer/buffer.h"
#include <iterator>

namespace vsh::cl {
    class cbuffer_view
    {
    public:
        class iterator
        {
            friend cbuffer_view;

            iterator(const std::byte *buffer) noexcept;

        public:
            using difference_type = std::ptrdiff_t;
            using value_type = std::byte;
            using pointer = const std::byte *;
            using reference = const std::byte &;
            using iterator_category = std::random_access_iterator_tag;

            iterator &operator++() noexcept;
            iterator operator++(int) noexcept;
            iterator &operator--() noexcept;
            iterator operator--(int) noexcept;

            const std::byte &operator*() const noexcept;

            bool operator==(const iterator &other) const noexcept;
            bool operator!=(const iterator &other) const noexcept;

            iterator operator+(difference_type offset) const noexcept;
            iterator operator-(difference_type offset) const noexcept;

            difference_type operator-(iterator other) const noexcept;

        private:
            const std::byte *m_buffer;
        };

        using const_iterator = iterator;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

        cbuffer_view() noexcept;
        cbuffer_view(const std::byte *buffer, size_t size) noexcept;
        cbuffer_view(const buffer &buf) noexcept;

        const std::byte *data() const noexcept;

        size_t size() const noexcept;

        const std::byte &operator[](size_t pos) const noexcept;

        operator bool() const noexcept;

        const std::byte &at(size_t pos) const;

        iterator begin() const noexcept;
        iterator end() const noexcept;

        const_iterator cbegin() const noexcept;
        const_iterator cend() const noexcept;

        reverse_iterator rbegin()const noexcept;
        reverse_iterator rend() const noexcept;

        const_reverse_iterator crbegin() const noexcept;
        const_reverse_iterator crend() const noexcept;

    private:
        const std::byte *m_buffer;
        size_t m_size;
    };

    cbuffer_view::iterator operator+(cbuffer_view::iterator::difference_type offset,
                                     const cbuffer_view::iterator &rhs) noexcept;
    cbuffer_view::reverse_iterator operator+(cbuffer_view::reverse_iterator::difference_type offset,
                                             const cbuffer_view::reverse_iterator &rhs) noexcept;

    bool operator==(const cbuffer_view &lhs, const cbuffer_view &rhs) noexcept;
    bool operator!=(const cbuffer_view &lhs, const cbuffer_view &rhs) noexcept;
}
