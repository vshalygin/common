#include "connection.h"
#include "rpc-lib/transfer-message/transfer-message.h"
#include "rpc-lib/types/constants.h"
#include "rpc-lib/transport/itransport.h"

#include <common-lib/syncronization/guarded-value/guarded-value.h>
#include <common-lib/timer/multiple-timer/multiple-timer.h>
#include <common-lib/syncronization/event/event.h>

#include <unordered_map>

namespace vshalygin::rpc {
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
        static std::shared_ptr<impl> create(std::shared_ptr<cl::thread_pool> thread_pool,
                                            std::shared_ptr<iservice> service,
                                            change_state_handler_t &&change_state_handler,
                                            const std::chrono::microseconds &timeout)
        {
            return std::make_shared<impl>(std::move(thread_pool),
                                          std::move(service),
                                          std::move(change_state_handler),
                                          timeout,
                                          creator());
        }

        impl(std::shared_ptr<cl::thread_pool> thread_pool,
             std::shared_ptr<iservice> service,
             change_state_handler_t &&change_state_handler,
             const std::chrono::microseconds &timeout,
             creator)
            : m_timeout(timeout)
            , m_thread_pool(std::move(thread_pool))
            , m_change_state_handler(std::move(change_state_handler))
            , m_multiple_timer(std::make_unique<cl::multiple_timer>(m_thread_pool->get_io_context()))
        {
            m_request_handler = [service = std::move(service)](cl::buffer &&buff,
                                                               response_handler_t &&res_handler) {
                if(service) {
                    service->process_request(std::move(buff), std::move(res_handler));
                }
            };
        }

        impl(impl &) = delete;
        impl &operator=(impl &) = delete;

        ~impl()
        {
            try {
                stop_transport();
                cancel_active_requests();
            } catch (...) {
                //TODO safe log
                std::terminate();
            }
        }

        void start_and_set_transport(std::unique_ptr<itransport> transport)
        {
            assert(transport);
            cl::event start_event;
            auto connect_handler = [&]() {
                call_change_state_handler(connection_state::connected);
                start_event.set();
            };
            auto disconnect_handler = [self = weak_from_this()]() {
                if(auto s = self.lock()) {
                    s->cancel_active_requests();
                    s->call_change_state_handler(connection_state::disconnected);
                }
            };

            transport->start(std::move(connect_handler), std::move(disconnect_handler));
            if(!start_event.wait_for(std::chrono::seconds(10))) {
                throw std::runtime_error("failed to wait transport starting");
            }
            transport->recv_async(create_receive_handler());

            auto [guard, curr_transport] = m_transport.get();
            curr_transport = std::move(transport);
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

        bool is_active() const
        {
            auto [guard, transport] = m_transport.get();
            return transport && transport->is_running();
        }

        void stop_transport()
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

        size_t get_active_timers_count() const
        {
            return m_multiple_timer->get_active_timers_count();
        }

    private:
        auto create_send_error_handler(uint64_t msg_number)
        {
            return [self = weak_from_this(), msg_number]() {
                if(auto s = self.lock()) {
                    s->handle_send_request_error(msg_number);
                }
            };
        }

        auto create_request_timout_handler(uint64_t msg_number)
        {
            return [self = weak_from_this(), msg_number]() {
                if(auto s = self.lock()) {
                    s->handle_request_timeout(msg_number);
                }
            };
        }

        auto create_receive_handler()
        {
            return [self = weak_from_this()](bool is_success, cl::buffer &&message) {
                if(auto s = self.lock()) {
                    if(is_success) s->dispatch_receive_event(std::move(message));
                    s->receive_async();
                }
            };
        }

        void add_request_handler_to_map(uint64_t msg_number,
                                        std::function<void(request_result, cl::buffer &&)> &&handler)
        {
            auto [guard, map] = m_request_map.get();
            assert(map.count(msg_number) == 0);
            auto timer_id = m_multiple_timer->start(create_request_timout_handler(msg_number), m_timeout);
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
                if(auto s = self.lock()) try {
                    auto [guard, transport] = s->m_transport.get();
                    assert(transport);
                    transport->send_async(std::move(res_msg), {});
                } catch(...) {
                    //TODO safe log
                }
            };
            auto task = [self = weak_from_this(), message = std::move(message),
                         response_handler = std::move(response_handler)]() mutable {
                if(auto s = self.lock()) try {
                    s->m_request_handler(std::move(message), std::move(response_handler));
                } catch (...) {
                    //TODO safe log
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
                    m_thread_pool->post([req_data, result, res_msg = std::move(res_msg)]() mutable {
                        try {
                            req_data->callback(result, std::move(res_msg));
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
            if(m_change_state_handler) try {
                m_change_state_handler(new_state);
            } catch (...) {
                //TODO log
            }
        }

    private:
        const std::chrono::microseconds m_timeout;
        std::shared_ptr<cl::thread_pool> m_thread_pool;
        std::function<void(connection_state)> m_change_state_handler;

        request_handler_t m_request_handler;
        cl::guarded_value<std::unique_ptr<itransport>> m_transport;
        std::unique_ptr<cl::multiple_timer> m_multiple_timer;

        cl::guarded_value<request_map> m_request_map;
    };

    connection::connection(std::shared_ptr<cl::thread_pool> thread_pool,
                           std::shared_ptr<iservice> service,
                           change_state_handler_t &&change_state_handler,
                           const std::chrono::microseconds &timeout)
        : m_impl(impl::create(std::move(thread_pool),
                              std::move(service),
                              std::move(change_state_handler),
                              timeout))
    {}

    connection::~connection() = default;

    void connection::start_and_set_transport(std::unique_ptr<itransport> transport)
    {
        m_impl->start_and_set_transport(std::move(transport));
    }

    void connection::request_async(cl::buffer &&message,
                                   std::function<void(request_result, cl::buffer &&)> &&handler)
    {
        m_impl->request_async(std::move(message), std::move(handler));
    }

    bool connection::is_active() const
    {
        return m_impl->is_active();
    }

    void connection::stop_transport()
    {
        m_impl->stop_transport();
    }

    size_t connection::get_active_requests_count() const
    {
        return m_impl->get_active_requests_count();
    }

    size_t connection::get_active_timers_count() const
    {
        return m_impl->get_active_timers_count();
    }
}
