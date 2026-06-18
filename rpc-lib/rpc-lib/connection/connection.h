#pragma once
#include "iconnection.h"
#include "rpc-lib/types/connection-state.h"
#include "common-lib/syncronization/guarded-value/guarded-value.h" //TODO sync
#include "common-lib/timer/multiple-timer/multiple-timer.h"

#include <memory>
#include <chrono>

namespace vshalygin::cl {
    class thread_pool;
}

namespace vshalygin::rpc {
    class iconnector;
    class transport;
    class iservice;

    class connection final
        : public iconnection
        , public std::enable_shared_from_this<connection>
    {
        class creator;

        struct request_data;

    public:
        using request_map = std::unordered_map<uint64_t, std::shared_ptr<request_data>>;
        using change_state_callback_t = std::function<void(connection_state)>;

        static std::shared_ptr<iconnection> create(std::shared_ptr<cl::thread_pool> thread_pool,
                                                   change_state_callback_t &&on_change_state,
                                                   std::shared_ptr<iconnector> connector,
                                                   std::shared_ptr<iservice> service,
                                                   const std::chrono::microseconds &req_timeout);

        connection(std::shared_ptr<cl::thread_pool> thread_pool,
                   change_state_callback_t &&on_change_state,
                   std::shared_ptr<iconnector> connector,
                   std::shared_ptr<iservice> service,
                   const std::chrono::microseconds &req_timeout,
                   creator);

        connection(const connection &) = delete;
        connection &operator=(const connection &) = delete;

        void activate() override;
        void deactivate() override;
        bool is_active() const override;

        void request_async(cl::buffer &&message,
                           request_callback_t &&callback) override;

    private:
        void do_receive_async();
        void dispatch_receive_event(cl::buffer &&message);
        void handle_received_request(cl::buffer &&message);
        void handle_received_response(cl::buffer &&message);

        void complete_request(uint64_t req_msg_number,
                              request_result result,
                              cl::buffer &&res_msg);

        void add_request_handler_to_map(uint64_t msg_number,
                                        request_callback_t &&handler);

        void remove_request_handler_from_map(uint64_t msg_number) noexcept;

    private:
        std::mutex m_activation_mtx;

        const std::chrono::microseconds m_req_timeout;

        std::shared_ptr<cl::thread_pool> m_thread_pool;
        std::shared_ptr<change_state_callback_t> m_on_change_state;
        std::shared_ptr<iconnector> m_connector;
        std::shared_ptr<iservice> m_service;

        cl::multiple_timer m_multiple_timer;

        cl::guarded_value<request_map> m_request_map;

        //transport must be destroyed first to complete any pending callbacks
        cl::guarded_value<std::unique_ptr<transport>> m_transport;
    };
}
