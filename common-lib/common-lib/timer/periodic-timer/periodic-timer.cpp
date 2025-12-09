#include "periodic-timer.h"

namespace vsh::cl {
    periodic_timer::periodic_timer(boost::asio::io_context &io_context)
        : m_io_context(io_context)
        , m_timer(m_io_context.get_executor())
    {}

    void periodic_timer::cancel()
    {
        m_is_canceled = true;
        m_timer.cancel();

        m_finish_event.wait();

        clear_state_to_initial();
    }

    bool periodic_timer::is_active() const
    {
        return m_is_active;
    }

    void periodic_timer::set_periods_count(size_t periods)
    {
        auto [guard, periods_count] = m_periods_count.get();
        periods_count = periods;
    }

    void periodic_timer::clear_periods_count()
    {
        auto [guard, periods_count] = m_periods_count.get();
        periods_count.reset();
    }

    bool periodic_timer::is_all_periods_completed() const
    {
        auto [guard, periods_count] = m_periods_count.get();
        return periods_count == m_current_periods_count;
    }

    void periodic_timer::increment_periods_count()
    {
        ++m_current_periods_count;
    }

    void periodic_timer::clear_state_to_initial()
    {
        m_current_periods_count = 0;
        m_is_canceled = false;
        m_finish_event.clear();

        //must be last
        m_is_active = false;
    }
}
