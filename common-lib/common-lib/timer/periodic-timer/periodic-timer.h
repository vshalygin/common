#pragma once
#include <common-lib/syncronization/guarded-value/guarded-value.h>
#include <common-lib/syncronization/event/event.h>

#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>

#include <chrono>
#include <optional>
#include <atomic>

namespace vsh::cl {
    class periodic_timer final
    {
    public:
        enum class callback_ret
        {
            Continue,
            Abort
        };

        explicit periodic_timer(boost::asio::io_context &io_context);

        periodic_timer(periodic_timer &) = delete;
        periodic_timer &operator=(periodic_timer &) = delete;

        ~periodic_timer();

        template<typename Callback>
        void start(Callback &&callback, const std::chrono::milliseconds &period);

        void cancel();

        bool is_active() const;

        void set_periods_count(size_t periods);
        void clear_periods_count();

    private:
        template<typename Callback>
        void start_period(Callback &&callback, const std::chrono::milliseconds &period);

        bool is_all_periods_completed() const;
        void increment_periods_count();

    private:
        boost::asio::io_context &m_io_context;
        boost::asio::steady_timer m_timer;

        cl::event m_finish_event;
        std::atomic_bool m_is_active = false;
        std::atomic_bool m_is_canceled = false;
        size_t m_current_periods_count = 0;

        cl::guarded_value<std::optional<size_t>> m_periods_count;
    };

    template<typename Callback>
    void periodic_timer::start(Callback &&callback, const std::chrono::milliseconds &period)
    {
        if(m_is_active.exchange(true)) {
            throw std::logic_error("periodic timer already started");
        }

        m_finish_event.clear();
        m_current_periods_count = 0;
        m_is_canceled = false;
        start_period<Callback>(std::move(callback), period);
    }

    template<typename Callback>
    void periodic_timer::start_period(Callback &&callback,
                                      const std::chrono::milliseconds &period)
    {
        //TODO check Callback signature

        m_timer.expires_after(period);
        m_timer.async_wait([this, period, callback = std::move(callback)]
                           (const boost::system::error_code &ec) mutable {
            if(!ec) {
                auto ret = callback_ret::Abort;
                try {
                    ret = callback();
                } catch(...) {
                    //TODO log
                }

                increment_periods_count();
                if(ret == callback_ret::Abort || m_is_canceled || is_all_periods_completed()) {
                    m_finish_event.set();
                    m_is_active = false;
                } else {
                    start_period<Callback>(std::move(callback), period);
                }
            } else if(ec == boost::asio::error::operation_aborted) {
                m_finish_event.set();
                m_is_active = false;
            } else {
                //TODO log something unexpected
                m_finish_event.set();
                m_is_active = false;
            }
        });
    }
}
