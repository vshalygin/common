#include "server-connector.h"

#include <rpc-lib/pipe/iserver-pipe-env.h>
#include <rpc-lib/pipe/ipipe-endpoint.h>
#include <rpc-lib/types/future.h>
#include <rpc-lib/authenticator/iauthenticator.h>
#include <rpc-lib/internal/connection/connection.h>
#include <rpc-lib/internal/service/iservice.h>

#include <common-lib/synchronization/value-locker.h>
#include <common-lib/thread/thread-pool/strand.h>

#include <mutex>
#include <unordered_map>

namespace vshalygin::rpc::internal {
    class server_connector::impl
        : public std::enable_shared_from_this<impl>
    {
        using connect_pipe_future = future<std::shared_ptr<ipipe_endpoint>>;
        using connection_future = future<void>;

        using create_service_t = std::function<std::unique_ptr<iservice>(uint64_t)>;
        using on_new_connection_t = std::function<void(uint64_t, std::unique_ptr<iconnection>)>;
        using on_change_state_t = std::function<void(server_connector_state)>;

    public:
        impl(std::shared_ptr<cl::thread_pool> thread_pool,
             std::shared_ptr<iauthenticator> authenticator,
             std::shared_ptr<iserver_pipe_env> pipe_env,
             create_service_t &&create_service,
             on_new_connection_t &&on_new_connection,
             on_change_state_t &&on_change_state,
             std::chrono::milliseconds handshake_timeout,
             std::chrono::milliseconds send_timeout,
             std::chrono::milliseconds recv_timeout,
             std::chrono::milliseconds check_period,
             std::chrono::milliseconds ping_timeout);

        impl(const impl &) = delete;
        impl &operator=(const impl &) = delete;

        void start();
        void stop();
        bool is_active() const;

        size_t get_pending_connections_count() const;

    private:
        void notify_on_start();
        void notify_on_stop();
        void notify_on_new_connection(uint64_t id, std::unique_ptr<iconnection> c, std::shared_ptr<bool> is_running_sp);

        void start_pipe_connect(bool first_time, std::shared_ptr<bool> is_running_sp);
        void stop_impl(std::shared_ptr<bool> is_running_sp);

        void erase_connection_future_from_map(uint64_t connection_id);
        void clear_connection_future_map();

    private:
        uint64_t m_next_connection_id = 0;

        mutable std::mutex m_mtx;
        std::shared_ptr<bool> m_is_running_sp;

        connect_pipe_future m_connect_pipe_future;

        std::shared_ptr<cl::thread_pool> m_thread_pool;
        cl::strand m_notify_strand;

        std::shared_ptr<iauthenticator> m_authenticator;
        std::shared_ptr<iserver_pipe_env> m_pipe_env;

        const create_service_t m_create_service;
        const on_new_connection_t m_on_new_connection;
        const on_change_state_t m_on_change_state;

        const std::chrono::milliseconds m_handshake_timeout;
        const std::chrono::milliseconds m_send_timeout;
        const std::chrono::milliseconds m_recv_timeout;
        const std::chrono::milliseconds m_check_period;
        const std::chrono::milliseconds m_ping_timeout;

        cl::value_locker<std::unordered_map<uint64_t, connection_future>> m_connection_future_map;
    };

    server_connector::impl::impl(std::shared_ptr<cl::thread_pool> thread_pool,
                                 std::shared_ptr<iauthenticator> authenticator,
                                 std::shared_ptr<iserver_pipe_env> pipe_env,
                                 create_service_t &&create_service,
                                 on_new_connection_t &&on_new_connection,
                                 on_change_state_t &&on_change_state,
                                 std::chrono::milliseconds handshake_timeout,
                                 std::chrono::milliseconds send_timeout,
                                 std::chrono::milliseconds recv_timeout,
                                 std::chrono::milliseconds check_period,
                                 std::chrono::milliseconds ping_timeout)
        : m_thread_pool(std::move(thread_pool))
        , m_notify_strand(m_thread_pool->get_io_context())
        , m_authenticator(std::move(authenticator))
        , m_pipe_env(std::move(pipe_env))
        , m_create_service(std::move(create_service))
        , m_on_new_connection(std::move(on_new_connection))
        , m_on_change_state(std::move(on_change_state))
        , m_handshake_timeout(handshake_timeout)
        , m_send_timeout(send_timeout)
        , m_recv_timeout(recv_timeout)
        , m_check_period(check_period)
        , m_ping_timeout(ping_timeout)
    {}

    void server_connector::impl::start()
    {
        start_pipe_connect(true, std::make_shared<bool>(true));
    }

    void server_connector::impl::stop()
    {
        std::shared_ptr<bool> is_running_sp;
        {
            std::lock_guard g(m_mtx);
            is_running_sp = m_is_running_sp;
        }

        stop_impl(std::move(is_running_sp));
    }

    bool server_connector::impl::is_active() const
    {
        std::lock_guard g(m_mtx);
        return m_is_running_sp && *m_is_running_sp;
    }

    size_t server_connector::impl::get_pending_connections_count() const
    {
        return m_connection_future_map.lock()->size();
    }

    void server_connector::impl::notify_on_start()
    {
        m_notify_strand.post([s = shared_from_this()]() {
            s->m_on_change_state(server_connector_state::started);
        });
    }

    void server_connector::impl::notify_on_stop()
    {
        m_notify_strand.post([s = shared_from_this()]() {
            s->m_on_change_state(server_connector_state::stopped);
        });
    }

    void server_connector::impl::notify_on_new_connection(
        uint64_t id, std::unique_ptr<iconnection> c, std::shared_ptr<bool> is_running_sp)
    {
        std::lock_guard g(m_mtx);
        if(is_running_sp && *is_running_sp) {
            m_notify_strand.post([s = shared_from_this(), id, c = std::move(c)]() mutable {
                s->m_on_new_connection(id, std::move(c));
            });
        }
    }

    void server_connector::impl::start_pipe_connect(bool first_time, std::shared_ptr<bool> is_running_sp)
    {
        std::lock_guard g(m_mtx);
        if(first_time) {
            if(m_is_running_sp && *m_is_running_sp) {
                throw std::logic_error("server connector already running");
            }

            notify_on_start();
            m_is_running_sp = is_running_sp;
        } else if(!is_running_sp || !*is_running_sp) {
            return;
        }

        m_connect_pipe_future = m_pipe_env->create_pipe()
                                    .then([](pipe_wait_res r, std::shared_ptr<ipipe_endpoint> ep) {
                                              if(is_fail(r)) {
                                                  throw std::runtime_error("pipe is not connected");
                                              }
                                              return std::move(ep);
                                          });

        auto connection_id = m_next_connection_id++;
        auto promise = make_promise(m_thread_pool.get(),
                                    [self = weak_from_this(), connection_id, is_running_sp]
                                    (std::shared_ptr<ipipe_endpoint> pe) {
            std::shared_ptr s(self);
            return pe->read_async(s->m_handshake_timeout)
                .then([pe, self](pipe_op_res r, cl::buffer &&b) {
                          if(is_fail(r)) {
                              throw std::runtime_error("read operation failed: " + to_string(r));
                          }
                          std::shared_ptr s(self);
                          if(!s->m_authenticator->check_request(b)) {
                              throw std::runtime_error("autentication error");
                          }
                          return pe->write_async(s->m_authenticator->create_response(b),
                                                 s->m_handshake_timeout);
                       })
                .then([pe, self, connection_id, is_running_sp](pipe_op_res r) mutable {
                          if(is_fail(r)){
                              throw std::runtime_error("write operation failed: " + to_string(r));
                          }

                          std::shared_ptr s(self);
                          auto c = std::make_unique<connection>(s->m_thread_pool,
                                                                std::move(pe),
                                                                s->m_create_service(connection_id),
                                                                s->m_send_timeout,
                                                                s->m_recv_timeout,
                                                                s->m_check_period,
                                                                s->m_ping_timeout);

                          s->notify_on_new_connection(connection_id, std::move(c), is_running_sp);
                      })
                .finally([self, connection_id]() {
                             if(auto s = self.lock()) {
                                 s->erase_connection_future_from_map(connection_id);
                             }
                         });
        });

        (*m_connection_future_map.lock())[connection_id] = promise.get_future();
        
        m_connect_pipe_future
            .then([promise = std::move(promise), self = shared_from_this(), is_running_sp]
                  (std::shared_ptr<ipipe_endpoint> ep) mutable {
                      promise.resolve(std::move(ep));
                      self->start_pipe_connect(false, is_running_sp);
                  })
            .catched([self = shared_from_this(), is_running_sp](std::exception_ptr) {
                         self->stop_impl(is_running_sp);
                     });
    }

    void server_connector::impl::stop_impl(std::shared_ptr<bool> is_running_sp)
    {
        std::lock_guard g(m_mtx);
        if(!is_running_sp || !*is_running_sp) {
            return;
        }
        *is_running_sp = false;
        m_is_running_sp.reset();

        m_pipe_env->cancel_pending_server_endpoints();
        clear_connection_future_map();
        m_connect_pipe_future = {};

        notify_on_stop();
    }

    void server_connector::impl::erase_connection_future_from_map(uint64_t connection_id)
    {
        connection_future to_delete;

        auto map = m_connection_future_map.lock();
        auto it = map->find(connection_id);
        if(it != map->end()) {
            to_delete = std::move(it->second);
            map->erase(it);
        }
    }

    void server_connector::impl::clear_connection_future_map()
    {
        std::vector<connection_future> to_delete;

        auto map = m_connection_future_map.lock();
        for(auto &el : *map) {
            to_delete.push_back(std::move(el.second));
        }
    }

    server_connector::server_connector(std::shared_ptr<cl::thread_pool> thread_pool,
                                       std::shared_ptr<iauthenticator> authenticator,
                                       std::shared_ptr<iserver_pipe_env> pipe_env,
                                       std::function<std::unique_ptr<iservice>(uint64_t)> &&create_service,
                                       std::function<void(uint64_t, std::unique_ptr<iconnection>)> &&on_new_connection,
                                       std::function<void(server_connector_state)> on_change_state,
                                       std::chrono::milliseconds handshake_timeout,
                                       std::chrono::milliseconds send_timeout,
                                       std::chrono::milliseconds recv_timeout,
                                       std::chrono::milliseconds check_period,
                                       std::chrono::milliseconds ping_timeout)
        : m_impl(std::make_shared<impl>(std::move(thread_pool), std::move(authenticator),
                                        std::move(pipe_env), std::move(create_service),
                                        std::move(on_new_connection), std::move(on_change_state),
                                        handshake_timeout, send_timeout, recv_timeout, check_period, ping_timeout))
    {}

    server_connector::~server_connector()
    {
        m_impl->stop();
    }

    void server_connector::start()
    {
        m_impl->start();
    }

    void server_connector::stop()
    {
        m_impl->stop();
    }

    bool server_connector::is_active() const
    {
        return m_impl->is_active();
    }

    size_t server_connector::get_pending_connections_count() const
    {
        return m_impl->get_pending_connections_count();
    }
}
