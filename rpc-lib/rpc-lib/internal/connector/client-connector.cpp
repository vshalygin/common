#include "client-connector.h"
#include "rpc-lib/pipe/ipipe-endpoint.h"
#include "rpc-lib/pipe/iclient-pipe-env.h"
#include "rpc-lib/authenticator/iauthenticator.h"
#include "rpc-lib/internal/connection/connection.h"

#include <stdexcept>
#include <atomic>

namespace vshalygin::rpc::internal {
    namespace {
        uint64_t generate_id() noexcept
        {
            static std::atomic_uint64_t next_id{ 0 };
            return next_id.fetch_add(1, std::memory_order_relaxed);
        }
    }

    class client_connector::impl
        : public std::enable_shared_from_this<impl>
    {
    public:
        explicit impl(cl::thread_pool *thread_pool,
                      std::shared_ptr<iauthenticator> authenticator,
                      std::shared_ptr<iclient_pipe_env> pipe_env,
                      const config &config);

        impl(const impl &) = delete;
        impl &operator=(const impl &) = delete;

        future<std::unique_ptr<iconnection>> create_connection_async(std::shared_ptr<iservice> service,
                                                                     std::chrono::milliseconds pipe_waiting_timeout);

        void cancel_connect_waiting();

    private:
        const uint64_t m_id;

        cl::thread_pool *m_thread_pool;
        std::shared_ptr<iauthenticator> m_authenticator;
        std::shared_ptr<iclient_pipe_env> m_pipe_env;
        const std::chrono::milliseconds m_handshake_timeout;
        const std::chrono::milliseconds m_send_timeout;
        const std::chrono::milliseconds m_recv_timeout;
        const std::chrono::milliseconds m_check_connection_period;
        const std::chrono::milliseconds m_ping_timeout;
    };

    client_connector::impl::impl(cl::thread_pool *thread_pool,
                                 std::shared_ptr<iauthenticator> authenticator,
                                 std::shared_ptr<iclient_pipe_env> pipe_env,
                                 const config &config)
        : m_id(generate_id())
        , m_thread_pool(thread_pool)
        , m_authenticator(std::move(authenticator))
        , m_pipe_env(std::move(pipe_env))
        , m_handshake_timeout(config.handshake_timeout)
        , m_send_timeout(config.send_timeout)
        , m_recv_timeout(config.recv_timeout)
        , m_check_connection_period(config.check_connection_period)
        , m_ping_timeout(config.ping_timeout)
    {}
    
    future<std::unique_ptr<iconnection>>
        client_connector::impl::create_connection_async(std::shared_ptr<iservice> service,
                                                        std::chrono::milliseconds pipe_waiting_timeout)
    {
        auto f = m_pipe_env->open_pipe(m_id, pipe_waiting_timeout)
            .then([self = weak_from_this(), service]
                  (pipe_wait_res r, std::shared_ptr<ipipe_endpoint> pipe_endpoint) {
                      if(is_fail(r)) {
                          throw std::runtime_error("pipe waiting failed");
                      }
                      std::shared_ptr s(self);
                      auto req = s->m_authenticator->create_request();
                      return  pipe_endpoint->write_async(std::move(req), s->m_handshake_timeout)
                          .then([pipe_endpoint, self] (pipe_op_res r) {
                                    if(is_fail(r)) {
                                        throw std::runtime_error("write operation failed: " + to_string(r));
                                    }
                                    std::shared_ptr s(self);
                                    return pipe_endpoint->read_async(s->m_handshake_timeout);
                                })
                          .then([pipe_endpoint, service, self](pipe_op_res r, cl::buffer &&b)
                                                                   -> std::unique_ptr<iconnection> {
                                    if(is_fail(r)) {
                                        throw std::runtime_error("read operation failed: " + to_string(r));
                                    }

                                    std::shared_ptr s(self);
                                    if(!s->m_authenticator->check_response(b)) {
                                        throw std::runtime_error("server forbid connection");
                                    }

                                    return std::make_unique<connection>(s->m_thread_pool,
                                                                        pipe_endpoint,
                                                                        service,
                                                                        s->m_send_timeout,
                                                                        s->m_recv_timeout,
                                                                        s->m_check_connection_period,
                                                                        s->m_ping_timeout);
                                });
                  });
            
        
        return f;
    }

    void client_connector::impl::cancel_connect_waiting()
    {
        m_pipe_env->cancel_pending_client_endpoints(m_id);
    }


    client_connector::client_connector(cl::thread_pool *thread_pool,
                                       std::shared_ptr<iauthenticator> authenticator,
                                       std::shared_ptr<iclient_pipe_env> pipe_env,
                                       const config &config)
        : m_impl(std::make_shared<impl>(thread_pool,
                                        std::move(authenticator),
                                        std::move(pipe_env),
                                        config))
    {}

    client_connector::~client_connector()
    {
        m_impl->cancel_connect_waiting();
    }

    future<std::unique_ptr<iconnection>>
        client_connector::create_connection_async(std::shared_ptr<iservice> service,
                                                  std::chrono::milliseconds pipe_waiting_timeout)
    {
        return m_impl->create_connection_async(std::move(service), pipe_waiting_timeout);
    }
}
