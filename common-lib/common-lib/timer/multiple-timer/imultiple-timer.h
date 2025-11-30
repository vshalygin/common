#pragma once
#include <functional>
#include <chrono>

namespace vsh::cl {
    class imultiple_timer
    {
    public:
        using callback_t = std::function<void()>;

        virtual ~imultiple_timer() = default;

        virtual uint64_t start(callback_t &&callback, const std::chrono::microseconds &microseconds) = 0;
        virtual void cancel(uint64_t id) = 0;
        virtual void cancel_all() = 0;

        virtual size_t get_active_timers_count() const = 0;
    };
}
