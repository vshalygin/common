#include "connection.h"
#include "rpc-lib/transfer-message/transfer-message.h"
#include "rpc-lib/types/constants.h"

#include <common-lib/syncronization/guarded-value/guarded-value.h>
#include <common-lib/timer/multiple-timer/multiple-timer.h>

#include <unordered_map>

namespace vsh::rpc {
    class connection::impl
        : public std::enable_shared_from_this<impl>
    {
        class creator
        {};

        using request_callback_t = std::function<void(request_result, cl::buffer &&)>;

        struct request_data
        {
            request_data(request_callback_t &&cb,
                         uint64_t id)
                : callback(std::move(cb))
                , timer_id(id)
            {}

            request_callback_t callback;
            uint64_t timer_id;
        };

        using request_map = std::unordered_map<uint64_t, std::shared_ptr<request_data>>;

    public:
        static std::shared_ptr<impl> create(std::unique_ptr<cl::imultiple_timer> multiple_timer,
                                            std::shared_ptr<cl::ithread_pool> thread_pool)
        {
            return std::make_shared<impl>(std::move(multiple_timer),
                                          std::move(thread_pool),
                                          creator());
        }

        impl(std::unique_ptr<cl::imultiple_timer> multiple_timer,
             std::shared_ptr<cl::ithread_pool> thread_pool,
             creator)
            : m_multiple_timer(std::move(multiple_timer))
            , m_thread_pool(std::move(thread_pool))
        {}

        impl(impl &) = delete;
        impl &operator=(impl &) = delete;

        ~impl()
        {
            try {
                disconnect();
                cancel_active_requests();
            } catch (...) {
                //TODO safe log
            }
        }

        void set_and_start_transport(std::unique_ptr<itransport> transport)
        {
            assert(transport);
            auto connect_handler = create_change_state_handler(connection_state::connected);
            auto disconnect_handler = create_change_state_handler(connection_state::disconnected);

            auto [guard, curr_transport] = m_transport.get();
            curr_transport = std::move(transport);
            curr_transport->set_start_callback(std::move(connect_handler));
            curr_transport->set_stop_callback(std::move(disconnect_handler));

            curr_transport->start();
            curr_transport->recv_async(create_receive_handler());
        }

        void request_async(cl::buffer &&message,
                           std::function<void(request_result, cl::buffer &&)> &&handler)
        {
            assert(get_transfer_msg_type(message) == transfer_msg_type::req);
            const auto msg_number = get_msg_number_req(message);

            try {
                add_request_handler_to_map(msg_number, std::move(handler));

                auto [guard, transport] = m_transport.get();
                if(transport) {
                    transport->send_async(std::move(message), create_send_error_handler(msg_number));
                } else {
                    m_thread_pool->post(create_send_error_handler(msg_number));
                }
                
            } catch (...){
                remove_request_handler_from_map(msg_number);
                throw;
            }
        }

        void set_request_handler
            (std::function<void(cl::buffer &&, response_handler_t &&)> &&handler)
        {
            auto [guard, request_handler] = m_request_handler.get();
            request_handler = std::move(handler);
        }

        void set_change_state_handler(std::function<void(connection_state)> &&handler)
        {
            auto [guard, change_state_handler] = m_change_state_handler.get();
            change_state_handler = std::move(handler);
        }

        bool is_connected() const
        {
            auto [guard, transport] = m_transport.get();
            return transport && !transport->is_stopped();
        }

        void disconnect()
        {
            auto [guard, transport] = m_transport.get();
            if(transport) {
                transport->stop();
            }
        }

        size_t get_active_requests_count() const
        {
            auto [guard, map] = m_request_map.get();
            return map.size();
        }

    private:
        std::function<void()> create_send_error_handler(uint64_t msg_number)
        {
            return [self = weak_from_this(), msg_number]() {
                if(auto s = self.lock()) {
                    s->handle_send_request_error(msg_number);
                }
            };
        }

        std::function<void()> create_request_timout_handler(uint64_t msg_number)
        {
            return [self = weak_from_this(), msg_number]() {
                if(auto s = self.lock()) {
                    s->handle_request_timeout(msg_number);
                }
            };
        }

        std::function<void(cl::buffer &&)> create_receive_handler()
        {
            return [self = weak_from_this()](cl::buffer &&message) {
                if(auto s = self.lock()) {
                    s->dispatch_receive_event(std::move(message));
                    s->receive_async();
                }
            };
        }

        void add_request_handler_to_map(uint64_t msg_number,
                                        std::function<void(request_result, cl::buffer &&)> &&handler)
        {
            auto [guard, map] = m_request_map.get();
            assert(map.count(msg_number) == 0);
            auto timer_id = m_multiple_timer->start(create_request_timout_handler(msg_number), RequestTimeout);
            map[msg_number] = std::make_shared<request_data>(std::move(handler), timer_id);
        }

        void remove_request_handler_from_map(uint64_t msg_number) noexcept
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
                    m_multiple_timer->cancel(req_data->timer_id);
                }
            } catch (...) {
                //TODO safe log
            }
        }

        void receive_async()
        {
            try {
                auto [guard, transport] = m_transport.get();
                assert(transport);
                transport->recv_async(create_receive_handler());
            } catch (...) {
                //TODO log;
            }
        }

        void dispatch_receive_event(cl::buffer &&message)
        {
            try {
                const auto message_type = get_transfer_msg_type(message);
                assert(message_type == transfer_msg_type::req || message_type == transfer_msg_type::res);
                if(transfer_msg_type::req == message_type) {
                    handle_request(std::move(message));
                } else if(transfer_msg_type::res == message_type) {
                    handle_response(std::move(message));
                }
            } catch (...) {
                //TODO safe log
            }
        }

        void handle_request(cl::buffer &&message)
        {
            auto response_handler = [self = weak_from_this()](cl::buffer &&res_msg) {
                if(auto s = self.lock()) {
                    try {
                        auto [guard, transport] = s->m_transport.get();
                        assert(transport);
                        transport->send_async(std::move(res_msg), {});
                    } catch(...) {
                        //TODO safe log
                    }
                }
            };
            auto sp_message = std::make_shared<cl::buffer>(std::move(message));

            auto task = [self = weak_from_this(), sp_message, response_handler]() {
                if(auto s = self.lock()) {
                    auto [guard, request_handler] = s->m_request_handler.get();
                    if(request_handler) try {
                        request_handler(std::move(*sp_message), response_handler);
                    } catch (...) {
                        //TODO safe log
                    }
                }
            };

            m_thread_pool->post(std::move(task));
        }

        bool complete_request(uint64_t req_msg_number,
                              request_result result,
                              cl::buffer &&res_msg)
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
                if(req_data->callback) {
                    auto sp_res_msg = std::make_shared<cl::buffer>(std::move(res_msg));
                    m_thread_pool->post([req_data, result, sp_res_msg]() {
                        try {
                            req_data->callback(result, std::move(*sp_res_msg));
                        } catch (...) {
                            //TODO log
                        }
                    });
                }
                m_multiple_timer->cancel(req_data->timer_id);
                return true;
            }
            return false;
        }

        void handle_response(cl::buffer &&message)
        {
            const auto message_number = get_msg_number_res(message);
            complete_request(message_number, request_result::ok, std::move(message));
        }

        void handle_send_request_error(uint64_t msg_number)
        {
            if(!complete_request(msg_number, request_result::send_error, {})) {
                //TODO log error
            }
        }

        void handle_request_timeout(uint64_t msg_number)
        {
            complete_request(msg_number, request_result::timeout, {});
        }

        void cancel_active_requests()
        {
            std::vector<uint64_t> request_ids;
            {
                auto [guard, request_map] = m_request_map.get();
                for(const auto &request_info : request_map) {
                    request_ids.push_back(request_info.first);
                }
            }
            for(auto id : request_ids) {
                complete_request(id, request_result::canceled, {});
            }
        }

        void call_change_state_handler(connection_state new_state)
        {
            auto [guard, change_state_handler] = m_change_state_handler.get();
            if(change_state_handler) {
                change_state_handler(new_state);
            }
        }

        std::function<void()> create_change_state_handler(connection_state new_state)
        {
            return [self = weak_from_this(), new_state]() {
                if(auto s = self.lock()) {
                    if(new_state == connection_state::disconnected) {
                        s->cancel_active_requests();
                    }
                    s->call_change_state_handler(new_state);
                }
            };
        }

    private:
        cl::guarded_value<std::unique_ptr<itransport>> m_transport;
        std::unique_ptr<cl::imultiple_timer> m_multiple_timer;
        std::shared_ptr<cl::ithread_pool> m_thread_pool;

        using request_handler_t = std::function<void(cl::buffer &&, response_handler_t &&)>;
        cl::guarded_value<request_handler_t> m_request_handler;

        cl::guarded_value<request_map> m_request_map;
        cl::guarded_value<std::function<void(connection_state)>> m_change_state_handler;
    };

    connection::connection(std::unique_ptr<cl::imultiple_timer> multiple_timer,
                           std::shared_ptr<cl::ithread_pool> thread_pool)
        : m_impl(impl::create(std::move(multiple_timer), std::move(thread_pool)))
    {}

    connection::connection(std::shared_ptr<cl::ithread_pool> thread_pool)
        : connection(std::make_unique<cl::multiple_timer>(*thread_pool->get_io_context()), thread_pool)
    {}

    connection::~connection() = default;

    void connection::set_and_start_transport(std::unique_ptr<itransport> transport)
    {
        m_impl->set_and_start_transport(std::move(transport));
    }

    void connection::request_async(cl::buffer &&message,
                                   std::function<void(request_result, cl::buffer &&)> &&handler)
    {
        m_impl->request_async(std::move(message), std::move(handler));
    }

    void connection::set_request_handler
        (std::function<void(cl::buffer &&, response_handler_t &&)> &&handler)
    {
        m_impl->set_request_handler(std::move(handler));
    }

    void connection::set_change_state_handler(std::function<void(connection_state)> &&handler)
    {
        m_impl->set_change_state_handler(std::move(handler));
    }

    bool connection::is_connected() const
    {
        return m_impl->is_connected();
    }

    void connection::disconnect()
    {
        return m_impl->disconnect();
    }

    size_t connection::get_active_requests_count() const
    {
        return m_impl->get_active_requests_count();
    }

    std::unique_ptr<iconnection> create_connection(std::unique_ptr<cl::imultiple_timer> multiple_timer,
                                                   std::shared_ptr<cl::ithread_pool> thread_pool)
    {
        return std::make_unique<connection>(std::move(multiple_timer), std::move(thread_pool));
    }
}
