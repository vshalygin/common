#pragma once
#include <chrono>
#include <memory>

namespace vsh::cl {
    class event final
    {
    public:
        explicit event(bool manual_reset = true,
                       bool initial_set = false);
        ~event();

        event(event &) = delete;
        event &operator=(event &) = delete;

        event(event &&) noexcept;
        event &operator=(event &&) noexcept;

        void set() noexcept;
        bool is_set() const noexcept;
        void reset() noexcept;

        void wait();
        bool wait_for(const std::chrono::microseconds &mcs);

    private:
        class impl;
        std::unique_ptr<impl> m_impl;
    };
}
