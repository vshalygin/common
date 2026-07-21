#include "multiple-timer.h"

namespace vshalygin::cl {
    multiple_timer::multiple_timer(boost::asio::io_context &io_context)
        : m_io_context(io_context)
        , m_timers_map(std::make_shared<value_locker<timers_map>>())
    {}

    multiple_timer::~multiple_timer()
    {
        cancel_all();
    }

    void multiple_timer::cancel(uint64_t id)
    {
        auto locked_map = m_timers_map->lock();
        auto it = locked_map->find(id);
        if(it != locked_map->end()) {
            it->second.cancel();
        }
    }

    void multiple_timer::cancel_all()
    {
        auto locked_map = m_timers_map->lock();
        for(auto &el : *locked_map) {
            el.second.cancel();
        }
    }

    size_t multiple_timer::get_active_timers_count() const
    {
        return m_timers_map->lock()->size();
    }
}
