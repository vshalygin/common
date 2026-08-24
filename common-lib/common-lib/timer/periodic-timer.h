#pragma once
#include <common-lib/utils/function.h>

#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>

#include <chrono>
#include <memory>
#include <vector>

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

        void stop_async(cl::function<void()> &&callback);

        bool is_active() const;

    private:
        class impl;
        std::shared_ptr<impl> m_impl;
    };

    class periodic_timer::impl
        : public std::enable_shared_from_this<impl>
    {
    public:
        explicit impl(boost::asio::io_context &io_context);

        impl(impl &) = delete;
        impl &operator=(impl &) = delete;

        void start(function<callback_ret()> &&callback,
                   std::chrono::milliseconds period,
                   size_t total_periods = static_cast<size_t>(-1));

        void stop_async(cl::function<void()> &&callback);

        bool is_active() const;

    private:
        void start_period(std::unique_ptr<function<callback_ret()>> &&func);

        void deactivate();

    private:
        boost::asio::io_context &m_io_context;
        boost::asio::steady_timer m_timer;

        mutable std::mutex m_mtx;
        bool m_is_active = false;
        bool m_is_stopped = false;
        size_t m_current_periods_count = 0;
        size_t m_total_periods_count = 0;
        std::chrono::milliseconds m_period;
        std::vector<cl::function<void()>> m_stop_callbacks;
    };

    template<typename Callback>
    void periodic_timer::start(Callback &&callback,
                               std::chrono::milliseconds period,
                               size_t total_periods)
    {
        return m_impl->start(std::forward<Callback>(callback), period, total_periods);
    }
}
