#include "client-connector.h"
#include "rpc-lib/pipe/ipipe-endpoint.h"
#include "rpc-lib/pipe/iclient-pipe-env.h"
#include "rpc-lib/authenticator/iauthenticator.h"
#include "rpc-lib/internal/connection/connection.h"

#include <stdexcept>

namespace vshalygin::rpc {
    class client_connector::impl
        : public std::enable_shared_from_this<impl>
    {
    public:
        explicit impl(std::shared_ptr<cl::thread_pool> thread_pool,
                      std::shared_ptr<iauthenticator> authenticator,
                      std::shared_ptr<iclient_pipe_env> pipe_env,
                      std::chrono::milliseconds handshake_timeout,
                      std::chrono::milliseconds send_timeout,
                      std::chrono::milliseconds recv_timeout);

        impl(const impl &) = delete;
        impl &operator=(const impl &) = delete;

        future<std::unique_ptr<iconnection>> create_connection_async(std::shared_ptr<iservice> service,
                                                                     std::chrono::milliseconds pipe_waiting_timeout);

        void cancel_connect_waiting();

    private:
        std::shared_ptr<cl::thread_pool> m_thread_pool;
        std::shared_ptr<iauthenticator> m_authenticator;
        std::shared_ptr<iclient_pipe_env> m_pipe_env;
        const std::chrono::milliseconds m_handshake_timeout;
        const std::chrono::milliseconds m_send_timeout;
        const std::chrono::milliseconds m_recv_timeout;
    };

    client_connector::impl::impl(std::shared_ptr<cl::thread_pool> thread_pool,
                                 std::shared_ptr<iauthenticator> authenticator,
                                 std::shared_ptr<iclient_pipe_env> pipe_env,
                                 std::chrono::milliseconds handshake_timeout,
                                 std::chrono::milliseconds send_timeout,
                                 std::chrono::milliseconds recv_timeout)
        : m_thread_pool(std::move(thread_pool))
        , m_authenticator(std::move(authenticator))
        , m_pipe_env(std::move(pipe_env))
        , m_handshake_timeout(handshake_timeout)
        , m_send_timeout(send_timeout)
        , m_recv_timeout(recv_timeout)
    {}
    
    future<std::unique_ptr<iconnection>>
        client_connector::impl::create_connection_async(std::shared_ptr<iservice> service,
                                                        std::chrono::milliseconds pipe_waiting_timeout)
    {
        auto promise = make_promise(m_thread_pool.get(), [self = shared_from_this(), pipe_waiting_timeout]() {
            return self->m_pipe_env->open_pipe(pipe_waiting_timeout);
        });
        auto future = promise.get_future()
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
                                                                        s->m_recv_timeout);
                                });
                  });
            
        
        promise.resolve();
        return future;
    }

    void client_connector::impl::cancel_connect_waiting()
    {
        m_pipe_env->cancel_pending_client_endpoints();
    }


    client_connector::client_connector(std::shared_ptr<cl::thread_pool> thread_pool,
                                       std::shared_ptr<iauthenticator> authenticator,
                                       std::shared_ptr<iclient_pipe_env> pipe_env,
                                       std::chrono::milliseconds handshake_timeout,
                                       std::chrono::milliseconds send_timeout,
                                       std::chrono::milliseconds recv_timeout)
        : m_impl(std::make_shared<impl>(std::move(thread_pool), std::move(authenticator),
                                        std::move(pipe_env),
                                        handshake_timeout, send_timeout, recv_timeout))
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
