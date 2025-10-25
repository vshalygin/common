#pragma once

namespace vsh::common_lib {
    class buffer_view
    {
    public:
        buffer_view() noexcept;
        buffer_view(unsigned char *buffer, size_t size) noexcept;

        unsigned char *data() noexcept;
        const unsigned char *data() const noexcept;

        size_t size() const noexcept;

        const unsigned char &operator[](size_t pos) const noexcept;
        unsigned char &operator[](size_t pos) noexcept;

    private:
        unsigned char *buffer_;
        size_t size_;
    };
}
