#pragma once
#include <chrono>
#include <memory>

namespace vsh::common_lib {
    class event
    {
    public:
        event() = default;
        ~event();

        event(event &) = delete;
        event &operator=(event &) = delete;

        event(event &&) = default;
        event &operator=(event &&) = default;

        void set();
        bool is_set();
        void clear();

        void wait();
        bool wait_for(const std::chrono::microseconds &mcs);

    private:
        class impl;
        std::unique_ptr<impl> impl_;
    };
}
