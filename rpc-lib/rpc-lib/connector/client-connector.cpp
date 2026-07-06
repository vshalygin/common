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
                                       std::chrono::milliseconds req_timeout,
                                       std::chrono::milliseconds res_timeout)
        : m_thread_pool(std::move(thread_pool))
        , m_authenticator(std::move(authenticator))
        , m_pipe_env(std::move(pipe_env))
        , m_service(std::move(service))
        , m_req_timeout(req_timeout)
        , m_res_timeout(res_timeout)
    {}

    future<connection> client_connector::create_connection_async(std::chrono::milliseconds timeout)
    {
        auto authenticator = m_authenticator;
        auto pipe_env = m_pipe_env;
        auto deadline = std::chrono::steady_clock::now() + timeout;
        auto thread_pool = m_thread_pool;
        auto service = m_service;
        auto req_timeout = m_req_timeout;
        auto res_timeout = m_res_timeout;

        auto promise = make_promise(thread_pool.get(),
            [timeout, pipe_env]() mutable {
                auto pipe_endpoint = pipe_env->open_pipe();
                auto res = pipe_endpoint->wait_connect_for(timeout);
                if(is_fail(res)) {
                    throw std::runtime_error("failed to wait pipe connection: " + to_string(res));
                }

                return pipe_endpoint;
            });
        promise.resolve();
        auto future = promise.get_future()
            .then([authenticator, deadline](std::shared_ptr<ipipe_endpoint> pipe_endpoint)
                  {
                      auto now = std::chrono::steady_clock::now();
                      if(now > deadline) {
                          throw std::runtime_error("timeout");
                      }
                      auto req = authenticator->create_request();
                      auto timeout = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - now);
                      return  pipe_endpoint->write_async(std::move(req), timeout)
                          .then([pipe_endpoint] (pipe_op_res r)
                                {
                                    if(is_fail(r)) {
                                        throw std::runtime_error("write operation failed: " + to_string(r));
                                    }
                                    return pipe_endpoint;
                                });
                  })
            .then([deadline, authenticator] (std::shared_ptr<ipipe_endpoint> pipe_endpoint) {
                      auto now = std::chrono::steady_clock::now();
                      if(now > deadline) {
                          throw std::runtime_error("timeout");
                      }
                      auto timeout = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - now);
                      return pipe_endpoint->read_async(timeout)
                          .then([pipe_endpoint, authenticator](pipe_op_res r, cl::buffer &&b)
                                {
                                    if(is_fail(r)) {
                                        throw std::runtime_error("read operation failed: " + to_string(r));
                                    }
                                    if(!authenticator->check_response(b)) {
                                        throw std::runtime_error("server forbid connection");
                                    }

                                    return pipe_endpoint;
                                });
                  })
            .then([thread_pool, service, req_timeout, res_timeout]
                  (std::shared_ptr<ipipe_endpoint> pipe_endpoint)
                  {
                      return connection(thread_pool,
                                        std::move(pipe_endpoint),
                                        service,
                                        req_timeout,
                                        res_timeout);
                  });

        return future;
    }
}
