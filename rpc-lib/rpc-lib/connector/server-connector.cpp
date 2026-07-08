#include "server-connector.h"
#include <rpc-lib/connection/connection.h>
#include <rpc-lib/pipe/iserver-pipe-env.h>
#include <rpc-lib/pipe/ipipe-endpoint.h>
#include <rpc-lib/service/iservice.h>
#include <rpc-lib/types/future.h>
#include <rpc-lib/authenticator/iauthenticator.h>
#include <common-lib/synchronization/guarded-value/guarded-value.h>

#include <mutex>
#include <unordered_map>

namespace vshalygin::rpc {
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
             std::chrono::milliseconds recv_timeout);

        impl(const impl &) = delete;
        impl &operator=(const impl &) = delete;

        void start();
        void stop();
        bool is_active() const;

        size_t get_pending_connections_count() const;

    private:
        void notify_on_start();
        void notify_on_stop();

        void start_pipe_connect(bool first_time);

        void erase_connection_future_from_map(uint64_t connection_id);

    private:
        uint64_t m_next_connection_id = 0;

        mutable std::mutex m_mtx;
        bool m_is_running = false;

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

        cl::guarded_value<std::unordered_map<uint64_t, connection_future>> m_connection_future_map;
    };

    server_connector::impl::impl(std::shared_ptr<cl::thread_pool> thread_pool,
                                 std::shared_ptr<iauthenticator> authenticator,
                                 std::shared_ptr<iserver_pipe_env> pipe_env,
                                 create_service_t &&create_service,
                                 on_new_connection_t &&on_new_connection,
                                 on_change_state_t &&on_change_state,
                                 std::chrono::milliseconds handshake_timeout,
                                 std::chrono::milliseconds send_timeout,
                                 std::chrono::milliseconds recv_timeout)
        : m_thread_pool(std::move(thread_pool))
        , m_notify_strand(m_thread_pool->create_strand())
        , m_authenticator(std::move(authenticator))
        , m_pipe_env(std::move(pipe_env))
        , m_create_service(std::move(create_service))
        , m_on_new_connection(std::move(on_new_connection))
        , m_on_change_state(std::move(on_change_state))
        , m_handshake_timeout(handshake_timeout)
        , m_send_timeout(send_timeout)
        , m_recv_timeout(recv_timeout)
    {}

    void server_connector::impl::start()
    {
        start_pipe_connect(true);
    }

    void server_connector::impl::stop()
    {
        std::lock_guard g(m_mtx);
        m_is_running = false;
        m_pipe_env->cancel_pending_server_endpoints();
        m_connect_pipe_future = {};
    }

    bool server_connector::impl::is_active() const
    {
        std::lock_guard g(m_mtx);
        return m_is_running;
    }

    size_t server_connector::impl::get_pending_connections_count() const
    {
        auto [guard, map] = m_connection_future_map.get();
        return map.size();
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

    void server_connector::impl::start_pipe_connect(bool first_time)
    {
        std::unique_lock g(m_mtx);
        if(first_time) {
            notify_on_start();
            m_is_running = true;
        } else if(!m_is_running) {
            notify_on_stop();
            return;
        }

        m_connect_pipe_future = m_pipe_env->create_pipe()
                                    .then([](pipe_op_res r, std::shared_ptr<ipipe_endpoint> ep) {
                                              if(is_fail(r)) {
                                                  throw std::runtime_error("pipe is not connected");
                                              }
                                              return std::move(ep);
                                          });

        auto connection_id = m_next_connection_id++;
        auto promise = make_promise(m_thread_pool.get(),
                                    [self = weak_from_this(), connection_id]
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
                .then([pe, self, connection_id](pipe_op_res r) mutable {
                          if(is_fail(r)){
                              throw std::runtime_error("write operation failed: " + to_string(r));
                          }

                          std::shared_ptr s(self);
                          auto c = std::make_unique<connection>(s->m_thread_pool,
                                                                std::move(pe),
                                                                s->m_create_service(connection_id),
                                                                s->m_send_timeout,
                                                                s->m_recv_timeout);

                          s->m_on_new_connection(connection_id, std::move(c));
                          s->erase_connection_future_from_map(connection_id);
                      })
                .catched([self, connection_id](std::exception_ptr) {
                             if(auto s = self.lock()) {
                                 s->erase_connection_future_from_map(connection_id);
                             }
                         });
            //TODO fix duplication erase after 'finally' method will be added to future interface
        });

        {
            auto [guard, map] = m_connection_future_map.get();
            map[connection_id] = promise.get_future();
        }
        
        m_connect_pipe_future
            .then([promise = std::move(promise), self = shared_from_this()]
                  (std::shared_ptr<ipipe_endpoint> ep) mutable {
                      promise.resolve(std::move(ep));
                      self->start_pipe_connect(false);
                  })
            .catched([self = shared_from_this()](std::exception_ptr) {
                         self->stop();
                         self->start_pipe_connect(false); //TODO use 'finally' here
                     });
    }

    void server_connector::impl::erase_connection_future_from_map(uint64_t connection_id)
    {
        //destroy future not under mutex to avoid any possible deadlocks
        connection_future f;

        auto [guard, map] = m_connection_future_map.get();
        auto it = map.find(connection_id);
        if(it != map.end()) {
            f = std::move(it->second);
            map.erase(it);
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
                                       std::chrono::milliseconds recv_timeout)
        : m_impl(std::make_shared<impl>(std::move(thread_pool), std::move(authenticator),
                                        std::move(pipe_env), std::move(create_service),
                                        std::move(on_new_connection), std::move(on_change_state),
                                        handshake_timeout, send_timeout, recv_timeout))
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
