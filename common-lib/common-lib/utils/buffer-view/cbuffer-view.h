#pragma once
namespace vsh::common_lib {
    class cbuffer_view
    {
    public:
        cbuffer_view(const unsigned char *buffer, size_t size);

        const unsigned char *data() const;

        size_t size() const;

    private:
        const unsigned char *buffer_;
        size_t size_;
    };
}
