#pragma once
#include "imultiple-timer.h"
#include <common-lib/utils/guarded-value/guarded-value.h>
#include <common-lib/utils/event/event.h>
#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <atomic>

namespace vsh::cl {
    class multiple_timer
        : public imultiple_timer
    {
    public:
        explicit multiple_timer(boost::asio::io_context &io_context);

        multiple_timer(multiple_timer &) = delete;
        multiple_timer &operator=(multiple_timer &) = delete;

        ~multiple_timer();

        uint64_t start(callback_t &&callback, const std::chrono::microseconds &microseconds) override;
        void cancel(uint64_t id) override;
        void cancel_all() override;

        size_t get_active_timers_count() const override;

    private:
        boost::asio::io_context &m_io_context;

        struct timer_struct
        {
            timer_struct(boost::asio::io_context &io_context)
                : timer(io_context)
            {}

            cl::event wait_event;
            boost::asio::steady_timer timer;
        };
        using timers_map = std::unordered_map<uint64_t, std::shared_ptr<timer_struct>>;
        cl::guarded_value<timers_map> m_timers_map;

        std::atomic<uint64_t> m_next_id = 0;
    };
}
