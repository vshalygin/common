#include "multiple-timer.h"

namespace vshalygin::cl {
    multiple_timer::multiple_timer(boost::asio::io_context &io_context)
        : m_io_context(io_context)
        , m_timers_map(std::make_shared<cl::guarded_value<timers_map>>())
    {}

    multiple_timer::~multiple_timer()
    {
        cancel_all();
    }

    void multiple_timer::cancel(uint64_t id)
    {
        auto [guard, timers_map] = m_timers_map->get();
        auto it = timers_map.find(id);
        if(it != timers_map.end()) {
            it->second.cancel();
        }
    }

    void multiple_timer::cancel_all()
    {
        auto [guard, timers_map] = m_timers_map->get();
        for(auto &el : timers_map) {
            el.second.cancel();
        }
    }

    size_t multiple_timer::get_active_timers_count() const
    {
        auto [guard, timers_map] = m_timers_map->get();
        return timers_map.size();
    }
}
