#include "connection.h"
#include "rpc-lib/transfer-message/transfer-message.h"

#include <common-lib/utils/guarded-value/guarded-value.h>

#include <unordered_map>

namespace vsh::rpc {
    namespace {
        const std::chrono::seconds s_timeout(10);
    }

    class connection::impl
        : public std::enable_shared_from_this<impl>
    {
        class creator
        {};

    public:
        static std::shared_ptr<impl> create(std::unique_ptr<itransport> transport,
                                            std::unique_ptr<cl::imultiple_timer> multiple_timer)
        {
            return std::make_shared<impl>(std::move(transport),
                                          std::move(multiple_timer),
                                          creator());
        }

        impl(std::unique_ptr<itransport> transport,
             std::unique_ptr<cl::imultiple_timer> multiple_timer,
             creator)
            : m_transport(std::move(transport))
            , m_multiple_timer(std::move(multiple_timer))
        {}

        impl(impl &) = delete;
        impl &operator=(impl &) = delete;

        void request_async(cl::buffer &&message,
                           std::function<void(request_result, cl::buffer &&)> &&handler)
        {
            assert(get_transfer_msg_type(message) == transfer_msg_type::req);
            const auto msg_number = get_msg_number_req(message);

            try {
                add_request_handler_to_map(msg_number, std::move(handler));

                m_multiple_timer->start(create_request_timout_handler(msg_number), s_timeout);
                m_transport->send_async(std::move(message), create_send_error_handler(msg_number));
            } catch (...){
                remove_request_handler_from_map(msg_number);
                throw;
            }
        }

        void start_receive_async()
        {
            receive_async();
        }

        void set_response_async_processor(
            std::function<void(cl::buffer &&, response_result_callback &&)> &&result_processor)
        {
            auto [guard, response_processor] = m_response_processor.get();
            response_processor = std::move(result_processor);
        }

        void set_disconnect_handler(std::function<void()> &&handler)
        {
            m_transport->set_stop_handler(std::move(handler));
        }

        bool is_connected() const
        {
            return !m_transport->is_stopped();
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

        void add_request_handler_to_map(uint64_t msg_number,
                                        std::function<void(request_result, cl::buffer &&)> &&handler)
        {
            auto [guard, map] = m_request_map.get();
            assert(map.count(msg_number) == 0);
            map[msg_number] = std::move(handler);
        }

        void remove_request_handler_from_map(uint64_t msg_number) noexcept
        {
            try {
                auto [guard, map] = m_request_map.get();
                map.erase(msg_number);
            } catch (...) {
                //TODO safe log
            }
        }

        void receive_async()
        {
            auto handler = [self = weak_from_this()](cl::buffer &&message) {
                if(auto s = self.lock()) {
                    s->dispatch_receive_event(std::move(message));
                    s->receive_async();
                }
            };

            m_transport->recv_async(std::move(handler));
        }

        void dispatch_receive_event(cl::buffer &&message)
        {
            const auto message_type = get_transfer_msg_type(message);
            assert(message_type == transfer_msg_type::req || message_type == transfer_msg_type::res);
            if(transfer_msg_type::req == message_type) {
                handle_request(std::move(message));
            } else if(transfer_msg_type::res == message_type) {
                handle_response(std::move(message));
            }
        }

        void handle_request(cl::buffer &&message)
        {
            auto callback = [self = weak_from_this()](cl::buffer &&message) {
                if(auto s = self.lock()) {
                    s->m_transport->send_async(std::move(message), {});
                }
            };
            try {
                auto [guard, response_handler] = m_response_processor.get();
                if(response_handler) {
                    response_handler(std::move(message), std::move(callback));
                }
            } catch (...) {
                //TODO log
            }
        }

        void handle_response(cl::buffer &&message)
        {
            auto message_number = get_msg_number_res(message);

            auto [guard, map] = m_request_map.get();
            auto it = map.find(message_number);
            if(it != map.end()) {
                auto &callback = it->second;
                try {
                    if(callback) {
                        callback(request_result::ok, std::move(message));
                    }
                } catch (...) {
                    //TODO log
                }
            }
        }

        void handle_send_request_error(uint64_t msg_number)
        {
            auto [guard, map] = m_request_map.get();
            auto it = map.find(msg_number);
            if(it != map.end()) {
                auto &callback = it->second;
                callback(request_result::send_error, {});

                map.erase(it);
            } else {
                //TODO log warn
            }
        }

        void handle_request_timeout(uint64_t msg_number)
        {
            auto [guard, map] = m_request_map.get();
            auto it = map.find(msg_number);
            if(it != map.end()) {
                auto &callback = it->second;
                callback(request_result::timeout, {});

                map.erase(it);
            }
        }

    private:
        std::unique_ptr<itransport> m_transport;
        std::unique_ptr<cl::imultiple_timer> m_multiple_timer;

        using response_processor = std::function<void(cl::buffer &&, response_result_callback &&)>;
        cl::guarded_value<response_processor> m_response_processor;

        using request_map = std::unordered_map<uint64_t, std::function<void(request_result, cl::buffer &&)>>;
        cl::guarded_value<request_map> m_request_map;
    };

    connection::connection(std::unique_ptr<itransport> transport,
                           std::unique_ptr<cl::imultiple_timer> multiple_timer)
        : m_impl(impl::create(std::move(transport), std::move(multiple_timer)))
    {
        m_impl->start_receive_async();
    }

    connection::~connection() = default;

    void connection::request_async(cl::buffer &&message,
                                   std::function<void(request_result, cl::buffer &&)> &&handler)
    {
        m_impl->request_async(std::move(message), std::move(handler));
    }

    void connection::set_response_async_processor(
        std::function<void(cl::buffer &&, response_result_callback &&)> &&processor)
    {
        m_impl->set_response_async_processor(std::move(processor));
    }

    void connection::set_disconnect_handler(std::function<void()> &&handler)
    {
        m_impl->set_disconnect_handler(std::move(handler));
    }

    bool connection::is_connected() const
    {
        return m_impl->is_connected();
    }
}
