#pragma once
#include <memory>
#include <iterator>

namespace vsh::cl {
    class buffer final
    {
    public:
        class iterator
        {
            friend buffer;

            iterator(unsigned char *buffer) noexcept;

        public:
            using difference_type = std::ptrdiff_t;
            using value_type = unsigned char;
            using pointer = unsigned char *;
            using reference = unsigned char &;
            using iterator_category = std::random_access_iterator_tag;

            iterator &operator++() noexcept;
            iterator operator++(int) noexcept;
            iterator &operator--() noexcept;
            iterator operator--(int) noexcept;

            unsigned char &operator*() noexcept;
            const unsigned char &operator*() const noexcept;

            bool operator==(const iterator &other) const noexcept;
            bool operator!=(const iterator &other) const noexcept;

            iterator operator+(difference_type offset) const noexcept;
            iterator operator-(difference_type offset) const noexcept;

            difference_type operator-(iterator other) const noexcept;

        private:
            unsigned char *m_buffer;
        };

        class const_iterator
        {
            friend buffer;

            const_iterator(const unsigned char *buffer) noexcept;

        public:
            using difference_type = std::ptrdiff_t;
            using value_type = unsigned char;
            using pointer = const unsigned char *;
            using reference = const unsigned char &;
            using iterator_category = std::random_access_iterator_tag;

            const_iterator &operator++() noexcept;
            const_iterator operator++(int) noexcept;
            const_iterator &operator--() noexcept;
            const_iterator operator--(int) noexcept;

            const unsigned char &operator*() const noexcept;

            bool operator==(const const_iterator &other) const noexcept;
            bool operator!=(const const_iterator &other) const noexcept;

            const_iterator operator+(difference_type offset) const noexcept;
            const_iterator operator-(difference_type offset) const noexcept;

            difference_type operator-(const_iterator other) const noexcept;

        private:
            const unsigned char *m_buffer;
        };

        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

        buffer() noexcept;
        explicit buffer(size_t size);

        ~buffer();

        buffer(const buffer &);
        buffer &operator=(const buffer &);

        buffer(buffer &&) noexcept;
        buffer &operator=(buffer &&) noexcept;

        unsigned char *data() noexcept;
        const unsigned char *data() const noexcept;

        size_t size() const noexcept;

        unsigned char &operator[](size_t pos) noexcept;
        const unsigned char &operator[](size_t pos) const noexcept;

        operator bool() const noexcept;

        unsigned char &at(size_t pos);
        const unsigned char &at(size_t pos) const;

        iterator begin() noexcept;
        iterator end() noexcept;

        const_iterator cbegin() const noexcept;
        const_iterator cend() const noexcept;

        reverse_iterator rbegin() noexcept;
        reverse_iterator rend() noexcept;

        const_reverse_iterator crbegin() const noexcept;
        const_reverse_iterator crend() const noexcept;

    private:
        unsigned char *m_buffer;
        size_t m_size;
    };

    buffer::iterator operator+(buffer::iterator::difference_type offset,
                               const buffer::iterator &rhs) noexcept;
    buffer::const_iterator operator+(buffer::const_iterator::difference_type offset,
                                     const buffer::const_iterator &rhs) noexcept;
    buffer::reverse_iterator operator+(buffer::reverse_iterator::difference_type offset,
                                       const buffer::reverse_iterator &rhs) noexcept;
    buffer::const_reverse_iterator operator+(buffer::const_reverse_iterator::difference_type offset,
                                             const buffer::const_reverse_iterator &rhs) noexcept;
}
