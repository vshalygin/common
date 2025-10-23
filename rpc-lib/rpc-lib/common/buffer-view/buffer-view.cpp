#include "buffer-view.h"

namespace vsh::rpc {
    buffer_view::buffer_view(unsigned char *buffer, size_t size)
        : buffer_(buffer)
        , size_(size)
    {}

    unsigned char *buffer_view::data()
    {
        return buffer_;
    }

    const unsigned char *buffer_view::data() const
    {
        return buffer_;
    }

    size_t buffer_view::size() const
    {
        return size_;
    }

    cbuffer_view::cbuffer_view(const unsigned char *buffer, size_t size)
        : buffer_(buffer)
        , size_(size)
    {}


    const unsigned char *cbuffer_view::data() const
    {
        return buffer_;
    }

    size_t cbuffer_view::size() const
    {
        return size_;
    }
}
