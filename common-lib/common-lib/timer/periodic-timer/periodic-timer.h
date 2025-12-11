#pragma once
#include <common-lib/syncronization/guarded-value/guarded-value.h>

#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>

#include <chrono>
#include <optional>
#include <atomic>
#include <condition_variable>

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

        void set_active_or_throw_if_already();
        void set_inactive_and_notify();

    private:
        boost::asio::io_context &m_io_context;
        boost::asio::steady_timer m_timer;

        mutable std::mutex m_is_active_mtx;
        std::condition_variable m_is_deactivated_cv;
        bool m_is_active = false;

        std::atomic_bool m_is_canceled = false;
        size_t m_current_periods_count = 0;

        cl::guarded_value<std::optional<size_t>> m_periods_count;
    };

    template<typename Callback>
    void periodic_timer::start(Callback &&callback, const std::chrono::milliseconds &period)
    {
        set_active_or_throw_if_already();

        m_current_periods_count = 0;
        m_is_canceled.store(false, std::memory_order_release);
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
                if(ret == callback_ret::Abort ||
                   m_is_canceled.load(std::memory_order_acquire) ||
                   is_all_periods_completed())
                {
                    set_inactive_and_notify();
                }
                else {
                    start_period<Callback>(std::move(callback), period);
                }
            } else if(ec == boost::asio::error::operation_aborted) {
                set_inactive_and_notify();
            } else {
                //TODO log something unexpected
                set_inactive_and_notify();
            }
        });
    }
}
