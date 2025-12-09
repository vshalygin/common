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
        explicit multiple_timer(boost::asio::io_context &io_context);

        multiple_timer(multiple_timer &) = delete;
        multiple_timer &operator=(multiple_timer &) = delete;

        ~multiple_timer();

        template<typename Callback>
        uint64_t start(Callback &&callback, const std::chrono::microseconds &microseconds);

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

    template<typename Callback>
    uint64_t multiple_timer::start(Callback &&callback,
                                   const std::chrono::microseconds &microseconds)
    {
        const auto timer_id = m_next_id.fetch_add(1);
        auto [guard, timers_map] = m_timers_map.get();

        auto timer_structure = std::make_shared<timer_struct>(m_io_context);
        timer_structure->timer.expires_after(microseconds);
        timer_structure->timer.async_wait([this, timer_id,
                                          callback = std::move(callback)]
                                          (const boost::system::error_code &ec) {
            std::shared_ptr<timer_struct> timer_struct;
            {
                auto [guard, timers_map] = m_timers_map.get();
                auto it = timers_map.find(timer_id);
                if(it != timers_map.end()) {
                    timer_struct = std::move(it->second);
                    timers_map.erase(it);
                }
                else {
                    //TODO log error
                }
            }

            if(!ec) {
                try {
                    callback();
                } catch(...) {
                    //TODO log
                }
            } else if(ec != boost::asio::error::operation_aborted) {
                //TODO log. something unexpected
            }

            if(timer_struct) {
                timer_struct->wait_event.set();
            }
        });

        assert(timers_map.count(timer_id) == 0);
        timers_map.insert(std::make_pair(timer_id, std::move(timer_structure)));

        return timer_id;
    }
}
