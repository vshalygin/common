#include "server-connector.h"

#include <rpc-lib/pipe/iserver-pipe-env.h>
#include <rpc-lib/pipe/ipipe-endpoint.h>
#include <rpc-lib/authenticator/iauthenticator.h>
#include <rpc-lib/internal/connection/connection.h>
#include <rpc-lib/internal/service/iservice.h>

#include <common-lib/synchronization/value-locker.h>
#include <common-lib/thread/thread-pool/strand.h>
#include <common-lib/thread/thread.h>

#include <mutex>
#include <unordered_map>

namespace vshalygin::rpc::internal {
    namespace {
        uint64_t generate_id() noexcept
        {
            static std::atomic_uint64_t next_id{ 0 };
            return next_id.fetch_add(1, std::memory_order_relaxed);
        }
    }

    class server_connector::impl
        : public std::enable_shared_from_this<impl>
    {
        using connect_pipe_future = cl::future<cl::thread_pool, std::shared_ptr<ipipe_endpoint>>;
        using connection_future = cl::future<cl::thread_pool, void>;

        using create_service_t = std::function<std::unique_ptr<iservice>(uint64_t)>;
        using on_new_connection_t = std::function<void(uint64_t, std::unique_ptr<iconnection>)>;
        using on_change_state_t = std::function<void(server_connector_state)>;

    public:
        impl(cl::thread_pool *thread_pool,
             std::shared_ptr<iauthenticator> authenticator,
             std::shared_ptr<iserver_pipe_env> pipe_env,
             create_service_t &&create_service,
             on_new_connection_t &&on_new_connection,
             on_change_state_t &&on_change_state,
             const config &config);

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
        const uint64_t m_id;

        uint64_t m_next_connection_id = 0;

        mutable std::mutex m_mtx;
        std::shared_ptr<bool> m_is_running_sp;

        connect_pipe_future m_connect_pipe_future;

        cl::thread_pool *m_thread_pool;
        cl::strand m_notify_strand;

        std::shared_ptr<iauthenticator> m_authenticator;
        std::shared_ptr<iserver_pipe_env> m_pipe_env;

        const create_service_t m_create_service;
        const on_new_connection_t m_on_new_connection;
        const on_change_state_t m_on_change_state;

        const config m_config;

        cl::value_locker<std::unordered_map<uint64_t, connection_future>> m_connection_future_map;
    };

    server_connector::impl::impl(cl::thread_pool *thread_pool,
                                 std::shared_ptr<iauthenticator> authenticator,
                                 std::shared_ptr<iserver_pipe_env> pipe_env,
                                 create_service_t &&create_service,
                                 on_new_connection_t &&on_new_connection,
                                 on_change_state_t &&on_change_state,
                                 const config &config)
        : m_id(generate_id())
        , m_thread_pool(thread_pool)
        , m_notify_strand(m_thread_pool->get_io_context())
        , m_authenticator(std::move(authenticator))
        , m_pipe_env(std::move(pipe_env))
        , m_create_service(std::move(create_service))
        , m_on_new_connection(std::move(on_new_connection))
        , m_on_change_state(std::move(on_change_state))
        , m_config(config)
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

        m_connect_pipe_future = m_pipe_env->create_pipe(m_id)
                                    .then([](pipe_wait_res r, std::shared_ptr<ipipe_endpoint> ep) {
                                              if(is_fail(r)) {
                                                  throw std::runtime_error("pipe is not connected");
                                              }
                                              return std::move(ep);
                                          });

        auto connection_id = m_next_connection_id++;
        cl::promise promise(m_thread_pool,
                           [self = weak_from_this(), connection_id, is_running_sp](std::shared_ptr<ipipe_endpoint> pe) {
            std::shared_ptr s(self);
            return pe->read_async(s->m_config.handshake_timeout)
                .then([pe, self](pipe_op_res r, cl::buffer &&b) {
                          if(is_fail(r)) {
                              throw std::runtime_error("read operation failed: " + to_string(r));
                          }
                          std::shared_ptr s(self);
                          if(!s->m_authenticator->check_request(b)) {
                              throw std::runtime_error("autentication error");
                          }
                          return pe->write_async(s->m_authenticator->create_response(b),
                                                 s->m_config.handshake_timeout);
                       })
                .then([pe, self, connection_id, is_running_sp](pipe_op_res r) mutable {
                          if(is_fail(r)){
                              throw std::runtime_error("write operation failed: " + to_string(r));
                          }

                          std::shared_ptr s(self);
                          auto c = std::make_unique<connection>(s->m_thread_pool,
                                                                std::move(pe),
                                                                s->m_create_service(connection_id),
                                                                s->m_config.send_timeout,
                                                                s->m_config.recv_timeout,
                                                                s->m_config.check_connection_period,
                                                                s->m_config.ping_timeout);

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

        m_pipe_env->cancel_pending_server_endpoints(m_id);
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
        map->clear();
    }

    server_connector::server_connector(cl::thread_pool *thread_pool,
                                       std::shared_ptr<iauthenticator> authenticator,
                                       std::shared_ptr<iserver_pipe_env> pipe_env,
                                       std::function<std::unique_ptr<iservice>(uint64_t)> &&create_service,
                                       std::function<void(uint64_t, std::unique_ptr<iconnection>)> &&on_new_connection,
                                       std::function<void(server_connector_state)> on_change_state,
                                       const config &config)
        : m_impl(std::make_shared<impl>(thread_pool, std::move(authenticator),
                                        std::move(pipe_env), std::move(create_service),
                                        std::move(on_new_connection), std::move(on_change_state),
                                        config))
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
