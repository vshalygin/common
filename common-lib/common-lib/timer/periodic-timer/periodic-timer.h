#pragma once
#include <common-lib/synchronization/value-locker.h>
#include <common-lib/utils/function.h>

#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>

#include <chrono>
#include <condition_variable>
#include <memory>

namespace vshalygin::cl {
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
        void start(Callback &&callback,
                   std::chrono::milliseconds period,
                   size_t total_periods = static_cast<size_t>(-1));

        void stop();

        bool is_active() const;

    private:
        void start_period(std::unique_ptr<function<callback_ret()>> &&func);

    private:
        boost::asio::io_context &m_io_context;
        boost::asio::steady_timer m_timer;

        mutable std::mutex m_mtx;
        std::condition_variable m_cv;
        bool m_is_active = false;
        bool m_is_stopped = false;
        size_t m_current_periods_count = 0;
        size_t m_total_periods_count = 0;
        std::chrono::milliseconds m_period;
    };

    template<typename Callback>
    void periodic_timer::start(Callback &&callback,
                               std::chrono::milliseconds period,
                               size_t total_periods)
    {
        std::lock_guard guard(m_mtx);
        if(m_is_active) {
            throw std::logic_error("periodic timer already running");
        }
        auto func = std::make_unique<function<callback_ret()>>(std::forward<Callback>(callback));

        m_is_active = true;
        m_is_stopped = false;
        m_current_periods_count = 0;
        m_total_periods_count = total_periods;
        m_period = period;

        start_period(std::move(func));
    }
}
