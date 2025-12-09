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
            //TODO log
            timer_struct->wait_event.wait();
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
