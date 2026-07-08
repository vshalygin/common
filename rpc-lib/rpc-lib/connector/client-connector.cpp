#include "client-connector.h"
#include "rpc-lib/pipe/ipipe-endpoint.h"
#include "rpc-lib/pipe/iclient-pipe-env.h"
#include "rpc-lib/authenticator/iauthenticator.h"
#include "rpc-lib/connection/connection.h"

#include <chrono>
#include <stdexcept>

namespace vshalygin::rpc {
    client_connector::client_connector(std::shared_ptr<cl::thread_pool> thread_pool,
                                       std::shared_ptr<iauthenticator> authenticator,
                                       std::shared_ptr<iclient_pipe_env> pipe_env,
                                       std::shared_ptr<iservice> service,
                                       std::chrono::milliseconds send_timeout,
                                       std::chrono::milliseconds recv_timeout)
        : m_thread_pool(std::move(thread_pool))
        , m_authenticator(std::move(authenticator))
        , m_pipe_env(std::move(pipe_env))
        , m_service(std::move(service))
        , m_send_timeout(send_timeout)
        , m_recv_timeout(recv_timeout)
    {}

    client_connector::~client_connector()
    {
        cancel_connect_waiting();
    }

    future<std::unique_ptr<iconnection>>
        client_connector::create_connection_async(std::chrono::milliseconds handshake_timeout)
    {
        auto authenticator = m_authenticator;
        auto pipe_env = m_pipe_env;
        auto thread_pool = m_thread_pool;
        auto service = m_service;
        auto send_timeout = m_send_timeout;
        auto recv_timeout = m_recv_timeout;
        
        //TODO make pretty
        auto promise = make_promise(thread_pool.get(), [pipe_env]() { return pipe_env->open_pipe(); });
        promise.resolve();
        auto future = promise.get_future()
            .then([authenticator, handshake_timeout]
                  (pipe_wait_res r, std::shared_ptr<ipipe_endpoint> pipe_endpoint) {
                      if(is_fail(r)) {
                          throw std::runtime_error("pipe waiting failed");
                      }
                      auto req = authenticator->create_request();
                      return  pipe_endpoint->write_async(std::move(req), handshake_timeout)
                          .then([pipe_endpoint] (pipe_op_res r) {
                                    if(is_fail(r)) {
                                        throw std::runtime_error("write operation failed: " + to_string(r));
                                    }
                                    return pipe_endpoint;
                                });
                  })
            .then([handshake_timeout, authenticator] (std::shared_ptr<ipipe_endpoint> pipe_endpoint) {
                      return pipe_endpoint->read_async(handshake_timeout)
                          .then([pipe_endpoint, authenticator](pipe_op_res r, cl::buffer &&b) {
                                    if(is_fail(r)) {
                                        throw std::runtime_error("read operation failed: " + to_string(r));
                                    }
                                    if(!authenticator->check_response(b)) {
                                        throw std::runtime_error("server forbid connection");
                                    }
        
                                    return pipe_endpoint;
                                });
                  })
            .then([thread_pool, service, send_timeout, recv_timeout]
                  (std::shared_ptr<ipipe_endpoint> pipe_endpoint) -> std::unique_ptr<iconnection>
                  {
                      return std::make_unique<connection>(thread_pool,
                                                          std::move(pipe_endpoint),
                                                          service,
                                                          send_timeout,
                                                          recv_timeout);
                  });
        
        return future;
    }

    void client_connector::cancel_connect_waiting()
    {
        m_pipe_env->cancel_pending_client_endpoints();
    }
}
