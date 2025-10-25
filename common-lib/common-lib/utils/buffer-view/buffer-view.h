#pragma once

namespace vsh::common_lib {
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
}
