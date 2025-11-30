#include "multiple-timer.h"

namespace vsh::cl {
    multiple_timer::multiple_timer(boost::asio::io_context &io_context)
        : m_io_context(io_context)
    {}

    multiple_timer::~multiple_timer()
    {
        try {
            //TODO log
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
        timer_structure->timer.expires_after(microseconds);
        timer_structure->timer.async_wait([this, timer_id,
                                           callback = std::move(callback)]
                                           (const boost::system::error_code &ec) {
            if(!ec) {
                try {
                    callback();
                } catch (...) {
                    //TODO log
                }
            } else if(ec != boost::asio::error::operation_aborted) {
                //TODO log. something unexpected
            }

            auto [guard, timers_map] = m_timers_map.get();
            auto it = timers_map.find(timer_id);
            if(it != timers_map.end()) {
                auto timer_struct = it->second;
                timers_map.erase(it);
                timer_struct->wait_event.set();
            }
        });

        assert(timers_map.count(timer_id) == 0);
        timers_map.insert(std::make_pair(timer_id, std::move(timer_structure)));

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
            if(!timer_struct->wait_event.wait_for(std::chrono::seconds(10))) {
                //TODO log
            }
        }
    }

    void multiple_timer::cancel_all()
    {
        std::vector<uint64_t> ids;

        {
            auto [guard, timers_map] = m_timers_map.get();
            for(auto &el : timers_map) {
                ids.push_back(el.first);
            }
        }

        for(auto id : ids) {
            try {
                cancel(id);
            } catch(...) {
                //TODO log
            }
        }
    }

    size_t multiple_timer::get_active_timers_count() const
    {
        auto [guard, timers_map] = m_timers_map.get();
        return timers_map.size();
    }
}
