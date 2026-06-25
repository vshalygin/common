#include "connection.h"
#include "rpc-lib/transport/transport.h"
#include "rpc-lib/connector/iconnector.h"
#include "rpc-lib/transfer-message/transfer-message.h"
#include "rpc-lib/service/iservice.h"

#include "common-lib/thread/thread-pool/thread-pool.h"
#include "common-lib/synchronization/event/event.h"

namespace vshalygin::rpc {
    namespace {
        void dummy_send_callback(pipe_op_res)
        {}
    }

    class connection::creator
    {};

    struct connection::request_data
    {
        request_data(request_callback_t &&cb,
                     uint64_t id)
            : callback(std::move(cb))
            , timer_id(id)
        {}

        request_callback_t callback;
        uint64_t timer_id;
    };

    std::shared_ptr<iconnection> connection::create(std::shared_ptr<cl::thread_pool> thread_pool,
                                                    std::shared_ptr<change_state_callback_t> on_change_state,
                                                    std::shared_ptr<iconnector> connector,
                                                    std::shared_ptr<iservice> service,
                                                    const std::chrono::milliseconds &req_timeout)
    {
        return std::make_shared<connection>(std::move(thread_pool),
                                            std::move(on_change_state),
                                            std::move(connector),
                                            std::move(service),
                                            req_timeout,
                                            creator{});
    };

    connection::connection(std::shared_ptr<cl::thread_pool> thread_pool,
                           std::shared_ptr<change_state_callback_t> on_change_state,
                           std::shared_ptr<iconnector> connector,
                           std::shared_ptr<iservice> service,
                           const std::chrono::milliseconds &req_timeout,
                           creator)
        : m_req_timeout(req_timeout)
        , m_thread_pool(std::move(thread_pool))
        , m_on_change_state(std::move(on_change_state))
        , m_connector(std::move(connector))
        , m_service(std::move(service))
        , m_multiple_timer(m_thread_pool->get_io_context())
    {}

    void connection::activate()
    {
        std::shared_ptr<cl::event> sync_event;
        auto start_callback = [on_change_state = m_on_change_state, sync_event]() {
            try {
                (*on_change_state)(connection_state::connected);
            } catch (...) { 
            }
            sync_event->set();
        };
        auto stop_callback = [on_change_state = m_on_change_state, sync_event]() {
            sync_event->wait();
            (*on_change_state)(connection_state::disconnected);
        };

        {
            std::lock_guard activation_guad(m_activation_mtx);
            auto transport = m_connector->create_transport(std::move(start_callback),
                                                           std::move(stop_callback));
            auto [guard, val] = m_transport.get();
            val = std::move(transport);
        }

        do_receive_async();
    }

    void connection::deactivate()
    {
        m_connector->interrupt();

        std::lock_guard activation_guard(m_activation_mtx);

        auto [guard, transport] = m_transport.get();
        if(transport) {
            transport->stop();
            transport.reset();
        }
    }

    bool connection::is_active() const
    {
        auto [guard, transport] = m_transport.get();
        return transport && transport->is_running();
    }

    void connection::request_async(cl::buffer &&message,
                                    request_callback_t &&callback)
    {
        assert(get_transfer_msg_type(message) == transfer_msg_type::req);
        const auto msg_number = get_msg_number_req(message);

        add_request_handler_to_map(msg_number, std::move(callback));

        auto req_callback = [self = weak_from_this(), msg_number](pipe_op_res res) {
            if(auto s = self.lock()) {
                if(res == pipe_op_res::canceled) {
                    s->complete_request(msg_number, request_result::canceled, {});
                } else if(is_fail(res)) {
                    s->complete_request(msg_number, request_result::send_error, {});
                }
            }
        };

        auto [guard, transport] = m_transport.get();
        if(transport) {
            transport->send_async(std::move(message), std::move(req_callback));
        } else {
            m_thread_pool->post([cb = std::move(req_callback),
                                 msg_number,
                                 self = weak_from_this()]()
            {
                if(auto s = self.lock()) {
                    s->complete_request(msg_number, request_result::send_error, {});
                }
            });
        }
    }

    void connection::do_receive_async()
    {
        auto [guard, transport] = m_transport.get();
        assert(transport);
        transport->recv_async([this](pipe_op_res r, cl::buffer &&message) {
            if(is_success(r)) {
                dispatch_receive_event(std::move(message));
                do_receive_async();
            }
        });
    }

    void connection::dispatch_receive_event(cl::buffer &&message)
    {
        const auto message_type = get_transfer_msg_type(message);
        assert(message_type == transfer_msg_type::req ||
               message_type == transfer_msg_type::res);

        if(transfer_msg_type::req == message_type) {
            handle_received_request(std::move(message));
        } else if(transfer_msg_type::res == message_type) {
            handle_received_response(std::move(message));
        }
    }

    void connection::handle_received_request(cl::buffer &&message)
    {
        auto response_handler = [self = weak_from_this()](cl::buffer &&res_msg) {
            if(auto s = self.lock()) {
                auto [guard, transport] = s->m_transport.get();
                assert(transport);
                transport->send_async(std::move(res_msg), &dummy_send_callback);
            }
        };
        auto task = [service = m_service, message = std::move(message),
                     response_handler = std::move(response_handler)]() mutable {
            if(service) {
                service->process_request(std::move(message),
                                         std::move(response_handler));
            }
        };

        m_thread_pool->post(std::move(task));
    }

    void connection::handle_received_response(cl::buffer &&message)
    {
        const auto message_number = get_msg_number_res(message);
        complete_request(message_number, request_result::ok, std::move(message));
    }

    void connection::complete_request(uint64_t req_msg_number,
                                       request_result result,
                                       cl::buffer && res_msg)
    {
        std::shared_ptr<request_data> req_data;

        {
            auto [guard, map] = m_request_map.get();
            auto it = map.find(req_msg_number);
            if(it != map.end()) {
                req_data = std::move(it->second);
                map.erase(it);
            }
        }

        if(req_data) {
            m_thread_pool->post([req_data, result, res_msg = std::move(res_msg)]() mutable {
                req_data->callback(result, std::move(res_msg));
            });
            m_multiple_timer.cancel(req_data->timer_id);
        }
    }

    void connection::add_request_handler_to_map(uint64_t msg_number,
                                                 request_callback_t &&handler)
    {
        auto timer_callback = [self = weak_from_this(), msg_number]() {
            if(auto s = self.lock()) {
                s->complete_request(msg_number, request_result::timeout, {});
            }
        };

        auto [guard, map] = m_request_map.get();
        assert(map.count(msg_number) == 0);
        auto timer_id = m_multiple_timer.start(std::move(timer_callback),
                                               m_req_timeout);
        map[msg_number] = std::make_shared<request_data>(std::move(handler), timer_id);
    }

    void connection::remove_request_handler_from_map(uint64_t msg_number) noexcept
    {
        try {
            std::shared_ptr<request_data> req_data;
            {
                auto [guard, map] = m_request_map.get();
                auto it = map.find(msg_number);
                if(it != map.end()) {
                    req_data = std::move(it->second);
                    map.erase(it);
                }
            }
            if(req_data) {
                m_multiple_timer.cancel(req_data->timer_id);
            }
        } catch(...) {
        }
    }
}
