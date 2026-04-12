#include "periodic-timer.h"

namespace vshalygin::cl {
    periodic_timer::periodic_timer(boost::asio::io_context &io_context)
        : m_io_context(io_context)
        , m_timer(m_io_context.get_executor())
    {}

    periodic_timer::~periodic_timer()
    {
        try {
            cancel();
        } catch (...) {
            //TODO safe log
        }
    }

    void periodic_timer::cancel()
    {
        std::unique_lock lock(m_is_active_mtx);
        if(!m_is_active) {
            return;
        }

        if(!m_is_canceled.exchange(true, std::memory_order_acq_rel)) {
            m_timer.cancel();
        }

        m_is_deactivated_cv.wait(lock, [this]() { return !m_is_active; });
    }

    bool periodic_timer::is_active() const
    {
        std::lock_guard lock(m_is_active_mtx);
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

    void periodic_timer::set_active_or_throw_if_already()
    {
        std::lock_guard lock(m_is_active_mtx);
        if(m_is_active) {
            throw std::logic_error("periodic timer already started");
        }
        m_is_active = true;
    }

    void periodic_timer::set_inactive_and_notify()
    {
        {
            std::lock_guard lock(m_is_active_mtx);
            m_is_active = false;
        }

        m_is_deactivated_cv.notify_all();
    }
}
