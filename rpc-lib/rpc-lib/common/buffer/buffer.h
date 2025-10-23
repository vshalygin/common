#pragma once

namespace vsh::rpc {
    class buffer final
    {
    public:
        buffer() noexcept;
        explicit buffer(size_t capacity);

        ~buffer();

        buffer(buffer &) = delete;
        buffer &operator=(buffer &) = delete;

        buffer(buffer &&) noexcept;
        buffer &operator=(buffer &&) noexcept;

        void reserve(size_t new_cap);

        char *data();
        const char *data() const;

        size_t capacity() const;

    private:
        char *buffer_;
        size_t capacity_;
    };
}
