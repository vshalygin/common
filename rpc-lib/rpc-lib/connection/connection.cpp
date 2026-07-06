#include "connection.h"
#include "rpc-lib/transport/transport.h"
#include "rpc-lib/transfer-message/transfer-message.h"
#include "rpc-lib/service/iservice.h"

#include "common-lib/thread/thread-pool/thread-pool.h"

namespace vshalygin::rpc {
    class connection::impl final
        : public std::enable_shared_from_this<impl>
    {
        struct request_data;

    public:
        using req_result_future = future<ftuple<request_result, cl::buffer>>;
        using req_result_promise = promise<ftuple<request_result, cl::buffer>, request_result, cl::buffer>;

        using request_map = std::unordered_map<uint64_t, std::shared_ptr<request_data>>; //TODO delete sp

        impl(std::shared_ptr<cl::thread_pool> thread_pool,
             std::shared_ptr<ipipe_endpoint> pipe_endpoint,
             std::shared_ptr<iservice> service,
             const std::chrono::milliseconds &req_timeout);

        impl(const impl &) = delete;
        impl &operator=(const impl &) = delete;

        void deactivate();
        bool is_active() const;

        req_result_future request_async(cl::buffer &&message);

        void set_stop_callback(std::function<void()> &&callback);

    private:
        void do_receive_async();
        void dispatch_receive_event(cl::buffer &&message);
        void handle_received_request(cl::buffer &&message);
        void handle_received_response(cl::buffer &&message);

        void complete_request(uint64_t req_msg_number,
                              request_result result,
                              cl::buffer &&res_msg);

        void add_request_to_map(uint64_t msg_number,
                                req_result_promise &&promise);

        void remove_request_from_map(uint64_t msg_number) noexcept;

    private:
        const std::chrono::milliseconds m_req_timeout;

        std::shared_ptr<cl::thread_pool> m_thread_pool;
        std::shared_ptr<iservice> m_service;

        cl::guarded_value<request_map> m_request_map;

        cl::multiple_timer m_multiple_timer;
        transport m_transport;
    };

    struct connection::impl::request_data
    {
        request_data(req_result_promise &&p,
                     uint64_t t_id)
            : promise(std::move(p))
            , timer_id(t_id)
        {}

        req_result_promise promise;
        uint64_t timer_id;
    };

    connection::impl::impl(std::shared_ptr<cl::thread_pool> thread_pool,
                           std::shared_ptr<ipipe_endpoint> pipe_endpoint,
                           std::shared_ptr<iservice> service,
                           const std::chrono::milliseconds &req_timeout)
        : m_req_timeout(req_timeout)
        , m_thread_pool(std::move(thread_pool))
        , m_service(std::move(service))
        , m_multiple_timer(m_thread_pool->get_io_context())
        , m_transport(std::move(pipe_endpoint))
    {}

    void connection::impl::deactivate()
    {
        m_transport.stop();
    }

    bool connection::impl::is_active() const
    {
        return m_transport.is_running();
    }

    connection::req_result_future connection::impl::request_async(cl::buffer &&message)
    {
        assert(get_transfer_msg_type(message) == transfer_msg_type::req);
        const auto msg_number = get_msg_number_req(message);

        auto promise = make_promise(m_thread_pool.get(), [](request_result r, cl::buffer b) {
            return ftuple(r, std::move(b));
        });
        auto future = promise.get_future();

        add_request_to_map(msg_number, std::move(promise));

        auto send_callback = [self = shared_from_this(), msg_number](pipe_op_res res) {
            if(res == pipe_op_res::canceled) {
                self->complete_request(msg_number, request_result::canceled, {});
            } else if(is_fail(res)) {
                self->complete_request(msg_number, request_result::send_error, {});
            }
        };

        m_transport.send_async(std::move(message))
            .then(std::move(send_callback));

        return future;
    }

    void connection::impl::set_stop_callback(std::function<void()> &&callback)
    {
        m_transport.set_stop_callback(std::move(callback));
    }

    void connection::impl::do_receive_async()
    {
        m_transport.recv_async()
            .then([self = shared_from_this()](pipe_op_res r, cl::buffer &&message) {
                      if(is_success(r)) {
                          self->dispatch_receive_event(std::move(message));
                          self->do_receive_async();
                      }
                  });
    }

    void connection::impl::dispatch_receive_event(cl::buffer &&message)
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

    void connection::impl::handle_received_request(cl::buffer &&message)
    {
        auto response_handler = [self = shared_from_this()](cl::buffer &&res_msg) mutable {
            self->m_transport.send_async(std::move(res_msg));
        };
        auto task = [service = m_service, message = std::move(message),
                     response_handler = std::move(response_handler)]() mutable {
            if(service) { //TODO use future here
                service->process_request(std::move(message),
                                         std::move(response_handler));
            }
        };

        m_thread_pool->post(std::move(task));
    }

    void connection::impl::handle_received_response(cl::buffer &&message)
    {
        const auto message_number = get_msg_number_res(message);
        complete_request(message_number, request_result::ok, std::move(message));
    }

    void connection::impl::complete_request(uint64_t req_msg_number,
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
            req_data->promise.resolve(result, std::move(res_msg));
            m_multiple_timer.cancel(req_data->timer_id);
        }
    }

    void connection::impl::add_request_to_map(uint64_t msg_number,
                                              req_result_promise &&promise)
    {
        auto timer_callback = [self = shared_from_this(), msg_number]() {
            self->complete_request(msg_number, request_result::timeout, {});
        };

        auto [guard, map] = m_request_map.get();
        assert(map.count(msg_number) == 0);
        auto timer_id = m_multiple_timer.start(std::move(timer_callback),
                                               m_req_timeout);
        map[msg_number] = std::make_shared<request_data>(std::move(promise), timer_id);
    }

    void connection::impl::remove_request_from_map(uint64_t msg_number) noexcept
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


    connection::connection(std::shared_ptr<cl::thread_pool> thread_pool,
                           std::shared_ptr<ipipe_endpoint> pipe_endpoint,
                           std::shared_ptr<iservice> service,
                           const std::chrono::milliseconds &req_timeout)
        : m_impl(std::make_shared<impl>(std::move(thread_pool),
                                        std::move(pipe_endpoint),
                                        std::move(service),
                                        req_timeout))
    {}

    connection::~connection()
    {
        deactivate();
    }

    void connection::deactivate()
    {
        m_impl->deactivate();
    }

    bool connection::is_active() const
    {
        return m_impl->is_active();
    }

    connection::req_result_future connection::request_async(cl::buffer &&message)
    {
        return m_impl->request_async(std::move(message));
    }

    void connection::set_stop_callback(std::function<void()> &&callback)
    {
        m_impl->set_stop_callback(std::move(callback));
    }
}
