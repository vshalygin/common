#pragma once
#include <common-lib/synchronization/spinlock/spinlock-guard.h>

#include <condition_variable>
#include <atomic>
#include <chrono>
#include <memory>

namespace vshalygin::cl {
    class event final
    {
    public:
        explicit event(bool manual_reset = true,
                       bool initial_set = false);

        event(const event &) = default;
        event &operator=(const event &) = default;

        event(event &&) = default;
        event &operator=(event &&) = default;

        void set() noexcept;
        bool is_set() const noexcept;
        void reset() noexcept;

        void wait();
        bool wait_for(std::chrono::milliseconds timeout);

    private:
        class impl;
        std::shared_ptr<impl> m_impl;
    };
}
