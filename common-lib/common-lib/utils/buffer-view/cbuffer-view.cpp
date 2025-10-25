#include "cbuffer-view.h"

namespace vsh::common_lib {
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
