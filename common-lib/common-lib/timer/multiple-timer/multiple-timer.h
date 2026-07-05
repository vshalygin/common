#pragma once
#include <common-lib/synchronization/guarded-value/guarded-value.h>
#include <common-lib/synchronization/event/event.h>
#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <atomic>

namespace vshalygin::cl {
    class multiple_timer final
    {
    public:
        explicit multiple_timer(boost::asio::io_context &io_context);

        multiple_timer(multiple_timer &) = delete;
        multiple_timer &operator=(multiple_timer &) = delete;

        ~multiple_timer();

        template<typename Callback>
        uint64_t start(Callback &&callback, const std::chrono::microseconds &microseconds);

        void cancel(uint64_t id);
        void cancel_all();

        size_t get_active_timers_count() const;

    private:
        boost::asio::io_context &m_io_context;

        using timers_map = std::unordered_map<uint64_t, boost::asio::steady_timer>;
        std::shared_ptr<cl::guarded_value<timers_map>> m_timers_map;

        std::atomic<uint64_t> m_next_id = 0;
    };

    template<typename Callback>
    uint64_t multiple_timer::start(Callback &&callback,
                                   const std::chrono::microseconds &microseconds)
    {
        const auto timer_id = m_next_id.fetch_add(1);
        auto [guard, timers_map] = m_timers_map->get();

        boost::asio::steady_timer timer(m_io_context);
        timer.expires_after(microseconds);
        timer.async_wait([timers_map_wp = std::weak_ptr(m_timers_map), timer_id,
                          callback = std::move(callback)]
                          (const boost::system::error_code &ec) mutable
        {
            if(auto timers_map = timers_map_wp.lock()){
                auto [guard, map] = timers_map->get();
                auto it = map.find(timer_id);
                if(it != map.end()) {
                    map.erase(it);
                }
            }

            if(!ec) try {
                callback();
            } catch(...) {
            }
        });

        assert(timers_map.count(timer_id) == 0);
        timers_map.insert(std::make_pair(timer_id, std::move(timer)));

        return timer_id;
    }
}
