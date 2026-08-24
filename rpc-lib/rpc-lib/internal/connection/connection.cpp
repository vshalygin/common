#include "connection.h"
#include "connection-watcher.h"

#include <rpc-lib/internal/transport/transport.h>
#include <rpc-lib/internal/transfer-message/transfer-message.h>
#include <rpc-lib/internal/service/iservice.h>
#include <rpc-lib/internal/transport/transport.h>

#include <common-lib/synchronization/value-locker.h>
#include <common-lib/timer/multiple-timer.h>
#include <common-lib/timer/periodic-timer.h>

#include <vector>

namespace vshalygin::rpc::internal {
    class connection::impl final
        : public std::enable_shared_from_this<impl>
    {
        struct request_data;

    public:
        using req_result_future = cl::future<cl::thread_pool, cl::ftuple<request_result, cl::buffer>>;
        using req_result_promise = cl::promise<cl::thread_pool, cl::ftuple<request_result, cl::buffer>(
                                                                                     request_result, cl::buffer)>;

        using request_map = std::unordered_map<uint64_t, request_data>;

        impl(cl::thread_pool *thread_pool,
             std::shared_ptr<ipipe_endpoint> pipe_endpoint,
             std::shared_ptr<iservice> service,
             std::chrono::milliseconds send_timeout,
             std::chrono::milliseconds recv_timeout,
             std::chrono::milliseconds check_period,
             std::chrono::milliseconds ping_timeout);

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
        void start_watching();

        void do_receive_async();
        void dispatch_receive_event(cl::buffer &&message) noexcept;
        void handle_received_request(cl::buffer &&message);
        void handle_received_response(cl::buffer &&message);
        void handle_received_ping();
        void handle_received_pong();

        void process_request_sent(uint64_t req_msg_number);

        void process_watch_event();

        void complete_request(uint64_t req_msg_number,
                              request_result result,
                              cl::buffer &&res_msg);

        void add_request_to_map(uint64_t msg_number,
                                req_result_promise &&promise);

        void complete_receive_routine(pipe_op_res r);

    private:
        mutable std::mutex m_mtx;
        enum class state
        {
            never_started,
            started,
            deactivated
        };
        state m_state = state::never_started;

        const std::chrono::milliseconds m_recv_timeout;
        const std::chrono::milliseconds m_check_period;

        cl::thread_pool *m_thread_pool;
        std::shared_ptr<iservice> m_service;

        cl::value_locker<request_map> m_request_map;

        cl::multiple_timer m_multiple_timer;
        cl::periodic_timer m_watcher_timer;

        transport m_transport;
        cl::value_locker<connection_watcher> m_watcher;
    };

    struct connection::impl::request_data
    {
        req_result_promise promise;
        uint64_t timer_id;

        bool is_req_sent;
        std::optional<request_result> fail_req_result;
    };

    connection::impl::impl(cl::thread_pool *thread_pool,
                           std::shared_ptr<ipipe_endpoint> pipe_endpoint,
                           std::shared_ptr<iservice> service,
                           std::chrono::milliseconds send_timeout,
                           std::chrono::milliseconds recv_timeout,
                           std::chrono::milliseconds check_period,
                           std::chrono::milliseconds ping_timeout)
        : m_recv_timeout(recv_timeout)
        , m_check_period(check_period)
        , m_thread_pool(thread_pool)
        , m_service(std::move(service))
        , m_multiple_timer(m_thread_pool->get_io_context())
        , m_watcher_timer(m_thread_pool->get_io_context())
        , m_transport(std::move(pipe_endpoint), send_timeout)
        , m_watcher(ping_timeout)
    {}

    void connection::impl::start()
    {
        std::lock_guard g(m_mtx);
        if(m_state == state::never_started) {
            m_state = state::started;
            start_watching();
            do_receive_async();
        }
    }

    void connection::impl::deactivate()
    {
        std::lock_guard g(m_mtx);
        m_state = state::deactivated;
        m_transport.stop();
    }

    bool connection::impl::is_active() const
    {
        std::lock_guard g(m_mtx);
        return m_state == state::started && m_transport.is_running();
    }

    connection::req_result_future connection::impl::request_async(cl::buffer &&message)
    {
        std::lock_guard g(m_mtx);
        if(m_state != state::started || !m_transport.is_running()) {
            return req_result_future(m_thread_pool, cl::ftuple(request_result::failed, cl::buffer{}));
        }

        assert(is_request_buffer_valid(message));
        assert(get_transfer_msg_type(message) == transfer_msg_type::req);
        const auto msg_number = get_msg_number_req(message);

        cl::promise promise(m_thread_pool, [](request_result r, cl::buffer b) {
            return cl::ftuple(r, std::move(b));
        });
        auto future = promise.get_future();

        add_request_to_map(msg_number, std::move(promise));

        auto send_callback = [self = shared_from_this(), msg_number](pipe_op_res res) {
            if(is_success(res)) {
                self->process_request_sent(msg_number);
            } else if(res == pipe_op_res::canceled) {
                self->complete_request(msg_number, request_result::send_canceled, {});
            } else if(res == pipe_op_res::timeout) {
                self->complete_request(msg_number, request_result::send_timeout, {});
            } else {
                self->complete_request(msg_number, request_result::send_failed, {});
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

    void connection::impl::start_watching()
    {
        m_watcher_timer.start([self = weak_from_this()]() {
            if(auto s = self.lock()) {
                s->process_watch_event();
                return cl::periodic_timer::callback_ret::Continue;
            }

            return cl::periodic_timer::callback_ret::Abort;
        }, m_check_period);
    }

    void connection::impl::do_receive_async()
    {
        m_transport.recv_async()
            .then([self = shared_from_this()](pipe_op_res r, cl::buffer &&message) {
                      if(is_success(r)) {
                          self->dispatch_receive_event(std::move(message));
                          self->do_receive_async();
                      } else {
                          self->complete_receive_routine(r);
                      }
                  });
    }

    void connection::impl::dispatch_receive_event(cl::buffer &&message) noexcept
    {
        try {
            switch (get_transfer_msg_type(message)) {
                case transfer_msg_type::req:
                    handle_received_request(std::move(message));
                    break;
                case transfer_msg_type::res:
                    handle_received_response(std::move(message));
                    break;
                case transfer_msg_type::ping:
                    handle_received_ping();
                    break;
                case transfer_msg_type::pong:
                    handle_received_pong();
                    break;
                default:
                    assert(!"unknown type of received message");
            }

            m_watcher.lock()->set_activity_flag();
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

    void connection::impl::handle_received_ping()
    {
        m_transport.send_async(create_transfer_msg_pong());
    }

    void connection::impl::handle_received_pong()
    {}

    void connection::impl::process_request_sent(uint64_t req_msg_number)
    {
        req_result_promise to_delete;

        auto map = m_request_map.lock();
        auto it = map->find(req_msg_number);
        if(it != map->end()) {
            auto &req_data = it->second;
            req_data.is_req_sent = true;
            if(req_data.fail_req_result) {
                assert(is_fail(*req_data.fail_req_result));
                req_data.promise.resolve(*req_data.fail_req_result, {});
                to_delete = std::move(req_data.promise);
                map->erase(it);
            }
        }
    }

    void connection::impl::process_watch_event()
    {
        auto watcher = m_watcher.lock();
        if(watcher->is_connection_not_responding()) {
            deactivate();
        } else if(!watcher->check_and_drop_activity_flag()) {
            m_transport.send_async(create_transfer_msg_ping());
            watcher->set_ping_waiting();
        }
    }

    void connection::impl::complete_request(uint64_t req_msg_number,
                                            request_result result,
                                            cl::buffer &&res_msg)
    {
        req_result_promise promise;

        {
            auto map = m_request_map.lock();
            auto it = map->find(req_msg_number);
            if(it != map->end()) {
                auto &req_data = it->second;
                promise = std::move(req_data.promise);
                m_multiple_timer.cancel(req_data.timer_id);
                map->erase(it);
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

        auto map = m_request_map.lock();
        assert(map->count(msg_number) == 0);
        auto timer_id = m_multiple_timer.start(std::move(timer_callback),
                                               m_recv_timeout);
        map->emplace(msg_number, request_data{ std::move(promise), timer_id, false, std::nullopt });
    }

    void connection::impl::complete_receive_routine(pipe_op_res r)
    {
        m_watcher_timer.stop_async({});

        assert(is_fail(r));
        assert(r != pipe_op_res::timeout);

        request_result req_result = (pipe_op_res::canceled == r) ?
                                     request_result::canceled :
                                     request_result::failed;

        std::vector<req_result_promise> to_delete;

        auto map = m_request_map.lock();
        for(auto it = map->begin(); it != map->end(); ) {
            auto &req_data = it->second;
            m_multiple_timer.cancel(req_data.timer_id);

            if(req_data.is_req_sent) {
                req_data.promise.resolve(req_result, {});
                to_delete.push_back(std::move(req_data.promise));
                it = map->erase(it);
            } else {
                req_data.fail_req_result = req_result;
                ++it;
            }
        }
    }

    size_t connection::impl::get_pending_requests_count() const
    {
        return m_request_map.lock()->size();
    }

    size_t connection::impl::get_active_timers_count() const
    {
        return m_multiple_timer.get_active_timers_count();
    }

    connection::connection(cl::thread_pool *thread_pool,
                           std::shared_ptr<ipipe_endpoint> pipe_endpoint,
                           std::shared_ptr<iservice> service,
                           std::chrono::milliseconds send_timeout,
                           std::chrono::milliseconds recv_timeout,
                           std::chrono::milliseconds check_period,
                           std::chrono::milliseconds ping_timeout)
        : m_impl(std::make_shared<impl>(thread_pool,
                                        std::move(pipe_endpoint),
                                        std::move(service),
                                        send_timeout,
                                        recv_timeout,
                                        check_period,
                                        ping_timeout))
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
