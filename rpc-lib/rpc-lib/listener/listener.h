#pragma once
#include <rpc-lib/types/connection-state.h>

#include <common-lib/thread/thread-pool/thread-pool.h>
#include <common-lib/synchronization/guarded-value/guarded-value.h>

#include <thread>
#include <unordered_map>
#include <utility>
#include <chrono>

namespace vshalygin::rpc {
    class connector;
    class iservice;
    class connection;
    class ichannel;

    class listener
    {
        using connection_change_callback_t = std::function<void(uint64_t, connection_state)>;

    public:
        explicit listener(std::shared_ptr<connector> connector,
                          std::shared_ptr<iservice> service,
                          std::shared_ptr<cl::thread_pool> thread_pool,
                          connection_change_callback_t &&connect_change_callback,
                          const std::chrono::milliseconds &request_timeout);

        listener(listener &) = delete;
        listener &operator=(listener &) = delete;

        ~listener();

        void start();
        void stop();
        bool is_stopped() const;

        using channels = std::vector<std::pair<uint64_t, std::shared_ptr<ichannel>>>;
        std::shared_ptr<ichannel> get_channel(uint64_t id) const;
        channels get_all_channels() const;

        void drop_connection(uint64_t id);
        void drop_all_connections();

    private:
        void create_new_active_connection();

    private:
        const std::chrono::milliseconds m_request_timeout;

        std::shared_ptr<connection> m_current_connection;

        uint64_t m_next_connection_id = 0;

        std::shared_ptr<connector> m_connector;
        std::shared_ptr<iservice> m_service;
        std::shared_ptr<cl::thread_pool> m_thread_pool;
        std::shared_ptr<connection_change_callback_t> m_on_connection_change;

        using guarded_channel_map_t =
            cl::guarded_value<std::unordered_map<uint64_t, std::pair<bool, std::shared_ptr<ichannel>>>>;
        std::shared_ptr<guarded_channel_map_t> m_channel_map;

        mutable std::mutex m_mtx;
        std::jthread m_listen_thread;
        bool m_is_running = false;
    };
}
