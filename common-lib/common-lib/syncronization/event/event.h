#pragma once
#include <chrono>
#include <memory>

namespace vsh::cl {
    class event final
    {
    public:
        explicit event(bool manual_reset = true);
        ~event();

        event(event &) = delete;
        event &operator=(event &) = delete;

        event(event &&);
        event &operator=(event &&);

        void set();
        bool is_set() const;
        void reset();

        void wait();
        bool wait_for(const std::chrono::microseconds &mcs);

    private:
        class impl;
        std::unique_ptr<impl> m_impl;
    };
}
