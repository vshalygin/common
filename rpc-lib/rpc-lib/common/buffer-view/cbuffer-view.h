#pragma once
namespace vsh::rpc {
    //TODO move to common lib
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
