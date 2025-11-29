#include "multiple-timer.h"

namespace vsh::cl {
    multiple_timer::multiple_timer(boost::asio::io_context &io_context)
        : m_io_context(io_context)
    {}

    multiple_timer::~multiple_timer()
    {
        try {
            cancel_all();
        } catch(...) {
            //TODO log
        }

    }

    uint64_t multiple_timer::start(callback_t &&callback, const std::chrono::microseconds &microseconds)
    {
        const auto timer_id = m_next_id.fetch_add(1);
        auto [guard, timers_map] = m_timers_map.get();

        auto timer_structure = std::make_shared<timer_struct>(m_io_context);
        timer_structure->timer.async_wait([this, timer_id,
                                           callback = std::move(callback)]
                                           (const boost::system::error_code &ec) {
            if(!ec) {
                callback();
            } else if(ec != boost::asio::error::operation_aborted) {
                //TODO log something unexpected
            }

            auto [guard, timers_map] = m_timers_map.get();
            auto it = timers_map.find(timer_id);
            assert(it != timers_map.end());
            it->second->wait_event.set();
            timers_map.erase(it);
        });

        timer_structure->timer.expires_after(microseconds);

        assert(timers_map.count(timer_id) == 0);
        timers_map[timer_id] = std::move(timer_structure);

        return timer_id;
    }

    void multiple_timer::cancel(uint64_t id)
    {
        std::shared_ptr<timer_struct> timer_struct;

        {
            auto [guard, timers_map] = m_timers_map.get();
            auto it = timers_map.find(id);
            if(it != timers_map.end()) {
                timer_struct = it->second;
            }
        }
        
        if(timer_struct) {
            timer_struct->timer.cancel();
            timer_struct->wait_event.wait();
        }
    }

    void multiple_timer::cancel_all()
    {
        auto [guard, timers_map] = m_timers_map.get();
        for(auto &el : timers_map) {
            cancel(el.first);
        }
    }
}
