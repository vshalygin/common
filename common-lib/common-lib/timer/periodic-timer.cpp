#include "periodic-timer.h"

#include <boost/asio/post.hpp>

namespace vshalygin::cl {
    periodic_timer::impl::impl(boost::asio::io_context &io_context)
        : m_io_context(io_context)
        , m_timer(io_context)
    {}

    void periodic_timer::impl::start(function<callback_ret()> &&callback,
                                     std::chrono::milliseconds period,
                                     size_t total_periods)
    {
        std::lock_guard guard(m_mtx);
        if(m_is_active) {
            throw std::logic_error("periodic timer already running");
        }
        auto func = std::make_unique<function<callback_ret()>>(std::move(callback));

        m_is_active = true;
        m_is_stopped = false;
        m_current_periods_count = 0;
        m_total_periods_count = total_periods;
        m_period = period;

        start_period(std::move(func));
    }

    void periodic_timer::impl::stop_async(cl::function<void()> &&callback)
    {
        std::unique_lock lock(m_mtx);
        if(!m_is_active) {
            if(callback) {
                boost::asio::post(m_io_context, std::move(callback));
            }
            return;
        }

        if(callback) {
            m_stop_callbacks.push_back(std::move(callback));
        }

        if(!m_is_stopped) {
            m_timer.cancel();
            m_is_stopped = true;
        }
    }

    bool periodic_timer::impl::is_active() const
    {
        std::lock_guard lock(m_mtx);
        return m_is_active;
    }

    void periodic_timer::impl::start_period(std::unique_ptr<function<callback_ret()>> &&func)
    {
        //m_mtx here must be locked before

        m_timer.expires_after(m_period);
        m_timer.async_wait([self = shared_from_this(), func = std::move(func)]
                           (const boost::system::error_code &ec) mutable {
            if(ec) {
                std::lock_guard guard(self->m_mtx);
                self->deactivate();
            } else {
                auto ret = callback_ret::Abort;
                try {
                    ret = (*func)();
                } catch(...) {
                }

                std::unique_lock guard(self->m_mtx);
                ++self->m_current_periods_count;

                if(ret == callback_ret::Continue &&
                   !self->m_is_stopped &&
                   self->m_current_periods_count < self->m_total_periods_count)
                {
                    self->start_period(std::move(func));
                }
                else
                {
                    self->deactivate();
                }
            }
        });
    }

    void periodic_timer::impl::deactivate()
    {
        m_is_active = false;
        for(auto &cb : m_stop_callbacks) {
            boost::asio::post(m_io_context, std::move(cb));
        }
        m_stop_callbacks.clear();
    }

    periodic_timer::periodic_timer(boost::asio::io_context &io_context)
        : m_impl(std::make_shared<impl>(io_context))
    {}

    periodic_timer::~periodic_timer()
    {
        stop_async({});
    }

    void periodic_timer::stop_async(cl::function<void()> &&callback)
    {
        m_impl->stop_async(std::move(callback));
    }

    bool periodic_timer::is_active() const
    {
        return m_impl->is_active();
    }
}
