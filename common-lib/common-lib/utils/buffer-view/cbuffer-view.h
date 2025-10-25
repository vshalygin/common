#pragma once
namespace vsh::common_lib {
    class cbuffer_view
    {
    public:
        cbuffer_view(const unsigned char *buffer, size_t size) noexcept;

        const unsigned char *data() const noexcept;

        size_t size() const noexcept;

        const unsigned char &operator[](size_t pos) const noexcept;

    private:
        const unsigned char *buffer_;
        size_t size_;
    };
}
