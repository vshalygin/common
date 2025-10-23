#pragma once

namespace vsh::rpc {
    //TODO move to common lib
    class buffer_view
    {
    public:
        buffer_view(unsigned char *buffer, size_t size);

        unsigned char *data();
        const unsigned char *data() const;

        size_t size() const;

    private:
        unsigned char *buffer_;
        size_t size_;
    };

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
