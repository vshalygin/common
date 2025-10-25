#pragma once
#include <memory>

namespace vsh::common_lib {
    class buffer final
    {
    public:
        buffer() noexcept;
        explicit buffer(size_t size);

        ~buffer();

        buffer(buffer &) = delete;
        buffer &operator=(buffer &) = delete;

        buffer(buffer &&) noexcept;
        buffer &operator=(buffer &&) noexcept;

        unsigned char *data() noexcept;
        const unsigned char *data() const noexcept;

        size_t size() const noexcept;

        unsigned char &operator[](size_t pos) noexcept;
        const unsigned char &operator[](size_t pos) const noexcept;

    private:
        unsigned char *buffer_;
        size_t size_;
    };
}
