#pragma once
#include <common-lib/syncronization/guarded-value/guarded-value.h>
#include <common-lib/syncronization/event/event.h>
#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <atomic>

namespace vsh::cl {
    class multiple_timer final
    {
    public:
        using callback_t = std::function<void()>;

        explicit multiple_timer(boost::asio::io_context &io_context);

        multiple_timer(multiple_timer &) = delete;
        multiple_timer &operator=(multiple_timer &) = delete;

        ~multiple_timer();

        uint64_t start(callback_t &&callback, const std::chrono::microseconds &microseconds);
        void cancel(uint64_t id);
        void cancel_all();

        size_t get_active_timers_count() const;

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
