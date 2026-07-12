#include "connection.h"
#include <rpc-lib/internal/transport/transport.h>
#include <rpc-lib/internal/transfer-message/transfer-message.h>
#include <rpc-lib/internal/service/iservice.h>
#include <rpc-lib/internal/transport/transport.h>

#include <common-lib/synchronization/guarded-value/guarded-value.h>
#include <common-lib/timer/multiple-timer/multiple-timer.h>
#include <common-lib/thread/thread-pool/thread-pool.h>

#include <atomic>

namespace vshalygin::rpc {
    class connection::impl final
        : public std::enable_shared_from_this<impl>
    {
        struct request_data;

    public:
        using req_result_future = future<ftuple<request_result, cl::buffer>>;
        using req_result_promise = promise<ftuple<request_result, cl::buffer>, request_result, cl::buffer>;

        using request_map = std::unordered_map<uint64_t, request_data>;

        impl(std::shared_ptr<cl::thread_pool> thread_pool,
             std::shared_ptr<ipipe_endpoint> pipe_endpoint,
             std::shared_ptr<iservice> service,
             const std::chrono::milliseconds &send_timeout,
             const std::chrono::milliseconds &recv_timeout);

        impl(const impl &) = delete;
        impl &operator=(const impl &) = delete;

        void start();

        void deactivate();
        bool is_active() const;

        req_result_future request_async(cl::buffer &&message);

        void set_stop_callback(cl::thread_pool_task<void()> &&callback);

        size_t get_pending_requests_count() const;
        size_t get_active_timers_count() const;

    private:
        void do_receive_async();
        void dispatch_receive_event(cl::buffer &&message) noexcept;
        void handle_received_request(cl::buffer &&message);
        void handle_received_response(cl::buffer &&message);

        void complete_request(uint64_t req_msg_number,
                              request_result result,
                              cl::buffer &&res_msg);

        void add_request_to_map(uint64_t msg_number,
                                req_result_promise &&promise);

        void remove_request_from_map(uint64_t msg_number) noexcept;

        void cancel_active_requests();

    private:
        std::atomic_bool m_is_started = false;

        const std::chrono::milliseconds m_recv_timeout;

        std::shared_ptr<cl::thread_pool> m_thread_pool;
        std::shared_ptr<iservice> m_service;

        cl::guarded_value<request_map> m_request_map;

        cl::multiple_timer m_multiple_timer;
        transport m_transport;
    };

    struct connection::impl::request_data
    {
        req_result_promise promise;
        uint64_t timer_id;
    };

    connection::impl::impl(std::shared_ptr<cl::thread_pool> thread_pool,
                           std::shared_ptr<ipipe_endpoint> pipe_endpoint,
                           std::shared_ptr<iservice> service,
                           const std::chrono::milliseconds &send_timeout,
                           const std::chrono::milliseconds &recv_timeout)
        : m_recv_timeout(recv_timeout)
        , m_thread_pool(std::move(thread_pool))
        , m_service(std::move(service))
        , m_multiple_timer(m_thread_pool->get_io_context())
        , m_transport(std::move(pipe_endpoint), send_timeout)
    {}

    void connection::impl::start()
    {
        if(!m_is_started.exchange(true, std::memory_order_acq_rel)) {
            do_receive_async();
        }
    }

    void connection::impl::deactivate()
    {
        m_transport.stop();
        cancel_active_requests();
    }

    bool connection::impl::is_active() const
    {
        return m_is_started.load(std::memory_order_acquire) && m_transport.is_running();
    }

    connection::req_result_future connection::impl::request_async(cl::buffer &&message)
    {
        if(!is_active()) {
            throw std::runtime_error("transport is not active");
        }

        assert(get_transfer_msg_type(message) == transfer_msg_type::req);
        const auto msg_number = get_msg_number_req(message);

        auto promise = make_promise(m_thread_pool.get(), [](request_result r, cl::buffer b) {
            return ftuple(r, std::move(b));
        });
        auto future = promise.get_future();

        add_request_to_map(msg_number, std::move(promise));

        auto send_callback = [self = shared_from_this(), msg_number](pipe_op_res res) {
            if(res == pipe_op_res::canceled) {
                self->complete_request(msg_number, request_result::send_canceled_error, {});
            } else if(res == pipe_op_res::timeout) {
                self->complete_request(msg_number, request_result::send_timeout_error, {});
            } else if(is_fail(res)) {
                self->complete_request(msg_number, request_result::send_unknown_error, {});
            }
        };

        m_transport.send_async(std::move(message))
            .then(std::move(send_callback));

        return future;
    }

    void connection::impl::set_stop_callback(cl::thread_pool_task<void()> &&callback)
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

    void connection::impl::dispatch_receive_event(cl::buffer &&message) noexcept
    {
        try {
            const auto message_type = get_transfer_msg_type(message);

            if(transfer_msg_type::req == message_type) {
                handle_received_request(std::move(message));
            }
            else if(transfer_msg_type::res == message_type) {
                handle_received_response(std::move(message));
            }
            else {
                assert(!"unknown type of received message");
            }
        } catch (...) {
        }
    }

    void connection::impl::handle_received_request(cl::buffer &&message)
    {
        if(!m_service) {
            return;
        }

        m_service->process_request_async(std::move(message))
            .then([self = weak_from_this()](cl::buffer &&res_msg) {
                      if(auto s = self.lock()) {
                          s->m_transport.send_async(std::move(res_msg));
                      }
                  });
    }

    void connection::impl::handle_received_response(cl::buffer &&message)
    {
        const auto message_number = get_msg_number_res(message);
        complete_request(message_number, request_result::ok, std::move(message));
    }

    void connection::impl::complete_request(uint64_t req_msg_number,
                                            request_result result,
                                            cl::buffer &&res_msg)
    {
        req_result_promise promise;

        {
            auto [guard, map] = m_request_map.get();
            auto it = map.find(req_msg_number);
            if(it != map.end()) {
                auto &req_data = it->second;
                promise = std::move(req_data.promise);
                m_multiple_timer.cancel(req_data.timer_id);
                map.erase(it);
            }
        }

        if(promise.is_valid()) {
            promise.resolve(result, std::move(res_msg));
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
                                               m_recv_timeout);
        map[msg_number] = request_data{ std::move(promise), timer_id };
    }

    void connection::impl::remove_request_from_map(uint64_t msg_number) noexcept
    {
        try {
            auto [guard, map] = m_request_map.get();
            auto it = map.find(msg_number);
            if(it != map.end()) {
                m_multiple_timer.cancel(it->second.timer_id);
                map.erase(it);
            }
        } catch(...) {
        }
    }

    void connection::impl::cancel_active_requests()
    {
        auto [guard, map] = m_request_map.get();
        for(auto &el : map) {
            auto &req_data = el.second;
            req_data.promise.resolve(request_result::canceled, {});
            m_multiple_timer.cancel(req_data.timer_id);
        }
        map.clear();
    }

    size_t connection::impl::get_pending_requests_count() const
    {
        auto [guard, map] = m_request_map.get();
        return map.size();
    }

    size_t connection::impl::get_active_timers_count() const
    {
        return m_multiple_timer.get_active_timers_count();
    }

    connection::connection(std::shared_ptr<cl::thread_pool> thread_pool,
                           std::shared_ptr<ipipe_endpoint> pipe_endpoint,
                           std::shared_ptr<iservice> service,
                           const std::chrono::milliseconds &send_timeout,
                           const std::chrono::milliseconds &recv_timeout)
        : m_impl(std::make_shared<impl>(std::move(thread_pool),
                                        std::move(pipe_endpoint),
                                        std::move(service),
                                        send_timeout,
                                        recv_timeout))
    {}

    connection::~connection()
    {
        m_impl->deactivate();
    }

    void connection::start()
    {
        m_impl->start();
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

    void connection::set_stop_callback(cl::thread_pool_task<void()> &&callback)
    {
        m_impl->set_stop_callback(std::move(callback));
    }

    size_t connection::get_pending_requests_count() const
    {
        return m_impl->get_pending_requests_count();
    }

    size_t connection::get_active_timers_count() const
    {
        return m_impl->get_active_timers_count();
    }
}
