#pragma once
#include "common-lib/utils/buffer/buffer.h"
#include <iterator>

namespace vsh::cl {
    class buffer_view
    {
    public:
        class iterator
        {
            friend buffer_view;

            iterator(std::byte *buffer) noexcept;

        public:
            using difference_type = std::ptrdiff_t;
            using value_type = std::byte;
            using pointer = std::byte *;
            using reference = std::byte &;
            using iterator_category = std::random_access_iterator_tag;

            iterator &operator++() noexcept;
            iterator operator++(int) noexcept;
            iterator &operator--() noexcept;
            iterator operator--(int) noexcept;

            std::byte &operator*() noexcept;
            const std::byte &operator*() const noexcept;

            bool operator==(const iterator &other) const noexcept;
            bool operator!=(const iterator &other) const noexcept;

            iterator operator+(difference_type offset) const noexcept;
            iterator operator-(difference_type offset) const noexcept;

            difference_type operator-(iterator other) const noexcept;

        private:
            std::byte *m_buffer;
        };

        class const_iterator
        {
            friend buffer_view;

            const_iterator(const std::byte *buffer) noexcept;

        public:
            using difference_type = std::ptrdiff_t;
            using value_type = std::byte;
            using pointer = const std::byte *;
            using reference = const std::byte &;
            using iterator_category = std::random_access_iterator_tag;

            const_iterator &operator++() noexcept;
            const_iterator operator++(int) noexcept;
            const_iterator &operator--() noexcept;
            const_iterator operator--(int) noexcept;

            const std::byte &operator*() const noexcept;

            bool operator==(const const_iterator &other) const noexcept;
            bool operator!=(const const_iterator &other) const noexcept;

            const_iterator operator+(difference_type offset) const noexcept;
            const_iterator operator-(difference_type offset) const noexcept;

            difference_type operator-(const_iterator other) const noexcept;

        private:
            const std::byte *m_buffer;
        };

        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

        buffer_view() noexcept;
        buffer_view(std::byte *buffer, size_t size) noexcept;
        buffer_view(buffer &buf) noexcept;

        std::byte *data() noexcept;
        const std::byte *data() const noexcept;

        size_t size() const noexcept;

        const std::byte &operator[](size_t pos) const noexcept;
        std::byte &operator[](size_t pos) noexcept;

        operator bool() const noexcept;

        std::byte &at(size_t pos);
        const std::byte &at(size_t pos) const;

        iterator begin() noexcept;
        iterator end() noexcept;

        const_iterator cbegin() const noexcept;
        const_iterator cend() const noexcept;

        reverse_iterator rbegin() noexcept;
        reverse_iterator rend() noexcept;

        const_reverse_iterator crbegin() const noexcept;
        const_reverse_iterator crend() const noexcept;

    private:
        std::byte *m_buffer;
        size_t m_size;
    };

    buffer_view::iterator operator+(buffer_view::iterator::difference_type offset,
                                    const buffer_view::iterator &rhs) noexcept;
    buffer_view::const_iterator operator+(buffer_view::const_iterator::difference_type offset,
                                          const buffer_view::const_iterator &rhs) noexcept;
    buffer_view::reverse_iterator operator+(buffer_view::reverse_iterator::difference_type offset,
                                            const buffer_view::reverse_iterator &rhs) noexcept;
    buffer_view::const_reverse_iterator operator+(buffer_view::const_reverse_iterator::difference_type offset,
                                                  const buffer_view::const_reverse_iterator &rhs) noexcept;

    bool operator==(const buffer_view &lhs, const buffer_view &rhs) noexcept;
    bool operator!=(const buffer_view &lhs, const buffer_view &rhs) noexcept;
}
