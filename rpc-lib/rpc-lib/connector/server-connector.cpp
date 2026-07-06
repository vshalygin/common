#include "server-connector.h"
#include "rpc-lib/pipe/ipipe-endpoint.h"
#include "rpc-lib/pipe/iserver-pipe-env.h"
#include "rpc-lib/authenticator/iauthenticator.h"
#include "rpc-lib/connection/connection.h"

#include <chrono>
#include <stdexcept>

namespace vshalygin::rpc {
    server_connector::server_connector(std::shared_ptr<cl::thread_pool> thread_pool,
                                       std::shared_ptr<iauthenticator> authenticator,
                                       std::shared_ptr<iserver_pipe_env> pipe_env,
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

    future<connection> server_connector::create_connection_async()
    {
        auto authenticator = m_authenticator;
        auto pipe_env = m_pipe_env;
        auto thread_pool = m_thread_pool;
        auto service = m_service;
        auto req_timeout = m_req_timeout;
        auto res_timeout = m_res_timeout;

        auto promise = make_promise(thread_pool.get(),
            [pipe_env]() mutable {
                auto pipe_endpoint = pipe_env->create_pipe();
                auto res = pipe_endpoint->wait_connect();
                if(is_fail(res)) {
                    throw std::runtime_error("failed to wait pipe connection: " + to_string(res));
                }

                return pipe_endpoint;
            });
        promise.resolve();
        auto future = promise.get_future()
            .then([](std::shared_ptr<ipipe_endpoint> pipe_endpoint)
                  {
                      return  pipe_endpoint->read_async()
                          .then([pipe_endpoint] (pipe_op_res r, cl::buffer &&b)
                                {
                                    if(is_fail(r)) {
                                        throw std::runtime_error("read operation failed: " + to_string(r));
                                    }
                                    return ftuple{ pipe_endpoint, std::move(b) };
                                });
                  })
            .then([authenticator] (std::shared_ptr<ipipe_endpoint> pipe_endpoint, cl::buffer &&b) {
                      return pipe_endpoint->write_async(authenticator->create_response(b),
                                                        std::chrono::seconds(1))
                          .then([pipe_endpoint](pipe_op_res r)
                                {
                                    if(is_fail(r)) {
                                        throw std::runtime_error("write operation failed: " + to_string(r));
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
