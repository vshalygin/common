#pragma once
namespace vsh::common_lib {
    class buffer final
    {
    public:
        buffer() noexcept;
        explicit buffer(size_t size);

        ~buffer();

        buffer(buffer &) = delete;
        buffer &operator=(buffer &) = delete;

        buffer(buffer &&) noexcept;
        buffer &operator=(buffer &&) noexcept;

        void reallocate(size_t new_size);

        unsigned char *data();
        const unsigned char *data() const;

        size_t size() const;

    private:
        unsigned char *buffer_;
        size_t size_;
    };
}
