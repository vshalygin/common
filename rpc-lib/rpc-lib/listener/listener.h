#pragma once
#include "ilistener.h"
#include <rpc-lib/types/connection-state.h>

#include <common-lib/thread-pool/thread-pool.h>
#include <common-lib/syncronization/guarded-value/guarded-value.h>

#include <thread>
#include <unordered_map>
#include <utility>

namespace vshalygin::rpc {
    class iconnector;
    class iservice;
    class iconnection;

    class listener
        : public ilistener
    {
        using connection_change_handler_t = std::function<void(uint64_t, connection_state)>;

    public:
        explicit listener(std::shared_ptr<iconnector> connector,
                          std::shared_ptr<iservice> service,
                          std::shared_ptr<cl::thread_pool> thread_pool,
                          connection_change_handler_t &&handler);

        listener(listener &) = delete;
        listener &operator=(listener &) = delete;

        ~listener();

        void start() override;
        void stop() override;
        bool is_stopped() const override;

        void set_change_state_handler(change_state_handler_t &&handler) override;

        std::shared_ptr<ichannel> get_channel(uint64_t id) const override;
        channels get_all_channels() const override;

        void drop_connection(uint64_t id) override;
        void drop_all_connections() override;

    private:
        void create_new_active_connection();

    private:
        std::shared_ptr<iconnection> m_current_connection;

        uint64_t m_next_connection_id = 0;

        std::shared_ptr<iconnector> m_connector;
        std::shared_ptr<iservice> m_service;
        std::shared_ptr<cl::thread_pool> m_thread_pool;
        std::shared_ptr<connection_change_handler_t> m_connection_change_handler;

        cl::guarded_value<change_state_handler_t> m_state_change_handler;

        using guarded_channel_map_t =
            cl::guarded_value<std::unordered_map<uint64_t, std::pair<bool, std::shared_ptr<ichannel>>>>;
        std::shared_ptr<guarded_channel_map_t> m_channel_map;

        mutable std::mutex m_mtx;
        std::jthread m_listen_thread;
        bool m_is_running = false;
    };
}
