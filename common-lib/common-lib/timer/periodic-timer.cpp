#include "periodic-timer.h"

namespace vshalygin::cl {
    periodic_timer::periodic_timer(boost::asio::io_context &io_context)
        : m_io_context(io_context)
        , m_timer(m_io_context.get_executor())
    {}

    periodic_timer::~periodic_timer()
    {
        stop();
    }

    void periodic_timer::stop()
    {
        std::unique_lock lock(m_mtx);
        if(!m_is_active) {
            return;
        }

        m_timer.cancel();
        m_is_stopped = true;

        m_cv.wait(lock, [this]() { return !m_is_active; });
    }

    bool periodic_timer::is_active() const
    {
        std::lock_guard lock(m_mtx);
        return m_is_active;
    }

    void periodic_timer::start_period(std::unique_ptr<function<callback_ret()>> &&func)
    {
        //m_mtx here must be locked before

        m_timer.expires_after(m_period);
        m_timer.async_wait([this, func = std::move(func)]
        (const boost::system::error_code &ec) mutable {
            if(ec) {
                std::lock_guard guard(m_mtx);
                m_is_active = false;

                //do under mutex intentionally,
                //must not allow other thread to set m_is_active in start
                m_cv.notify_all();
            } else {
                auto ret = callback_ret::Abort;
                try {
                    ret = (*func)();
                } catch(...) {
                }

                std::unique_lock guard(m_mtx);
                ++m_current_periods_count;

                if(ret == callback_ret::Continue && !m_is_stopped &&
                   m_current_periods_count < m_total_periods_count)
                {
                    start_period(std::move(func));
                }
                else
                {
                    m_is_active = false;

                    //do under mutex intentionally,
                    //must not allow other thread to set m_is_active in start
                    m_cv.notify_all();
                }
            }
        });
    }
}
