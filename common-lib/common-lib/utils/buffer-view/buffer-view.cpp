#include "buffer-view.h"

namespace vsh::common_lib {
    buffer_view::buffer_view() noexcept
        : buffer_view(nullptr, 0)
    {}

    buffer_view::buffer_view(unsigned char *buffer, size_t size) noexcept
        : buffer_(buffer)
        , size_(size)
    {}

    unsigned char *buffer_view::data() noexcept
    {
        return const_cast<unsigned char *>(static_cast<const buffer_view &>(*this).data());
    }

    const unsigned char *buffer_view::data() const noexcept
    {
        return buffer_;
    }

    size_t buffer_view::size() const noexcept
    {
        return size_;
    }

    const unsigned char &buffer_view::operator[](size_t pos) const noexcept
    {
        return *(buffer_ + pos);
    }

    unsigned char &buffer_view::operator[](size_t pos) noexcept
    {
        return const_cast<unsigned char &>(static_cast<const buffer_view &>(*this)[pos]);
    }
}
