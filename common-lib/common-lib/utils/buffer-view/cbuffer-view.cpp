#include "cbuffer-view.h"

namespace vsh::common_lib {
    cbuffer_view::cbuffer_view(const unsigned char *buffer, size_t size) noexcept
        : buffer_(buffer)
        , size_(size)
    {}

    const unsigned char *cbuffer_view::data() const noexcept
    {
        return buffer_;
    }

    size_t cbuffer_view::size() const noexcept
    {
        return size_;
    }

    const unsigned char &cbuffer_view::operator[](size_t pos) const noexcept
    {
        return *(buffer_ + pos);
    }
}
