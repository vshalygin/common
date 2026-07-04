#include "server-connector.h"
#include "rpc-lib/transport/transport.h"
#include "rpc-lib/pipe/ipipe-endpoint.h"
#include "rpc-lib/pipe/ipipe-env.h"
#include "rpc-lib/authenticator/iauthenticator.h"
#include "rpc-lib/types/interrupt-exception.h"

#pragma warning(push, 0)
#include "rpc-lib/authenticator/proto/auth.pb.h"
#pragma warning(pop)

#include <chrono>
#include <stdexcept>

namespace vshalygin::rpc {
    server_connector::server_connector(std::shared_ptr<cl::thread_pool> thread_pool,
                                       std::shared_ptr<iauthenticator> authenticator,
                                       std::shared_ptr<ipipe_env> pipe_env)
        : m_thread_pool(std::move(thread_pool))
        , m_authenticator(std::move(authenticator))
        , m_pipe_env(std::move(pipe_env))
    {}

    std::unique_ptr<transport> server_connector::create_transport
                                                     (std::function<void()> &&/*start_callback*/,
                                                      std::function<void()> &&/*stop_callback*/) const
    {
        std::unique_lock guard(m_mtx);
        assert(!m_curr_pipe_endpoint);

        try {
            //TODO fix
            //m_curr_pipe_endpoint = m_pipe_env->create_pipe();
            //guard.unlock();
            //auto wait_res = m_curr_pipe_endpoint->wait_connect_for(std::chrono::seconds(10));
            //guard.lock();
            //
            //if(is_fail(wait_res)) {
            //    if(wait_res == pipe_wait_res::invalidated) {
            //        throw interrupt_exception("pipe connect process was interrupted");
            //    } else {
            //        throw std::runtime_error("failed to wait pipe connection: " + to_string(wait_res));
            //    }
            //}
            //
            //auto con_msg = m_curr_pipe_endpoint->try_to_read_for(std::chrono::seconds(10));
            //if(!con_msg) {
            //    throw std::runtime_error("failed to read handshake request");;
            //}
            //proto::auth_request req;
            //if(!req.ParseFromArray(con_msg->data(), static_cast<int>(con_msg->size()))) {
            //    throw std::runtime_error("failed to parse handshake request");
            //}
            //
            //if(!m_authenticator->check_request(req)) {
            //    throw std::runtime_error("handshake failed");
            //}
            //
            //proto::auth_response res;
            //res.set_is_accepted(true);
            //
            //cl::buffer buf(res.ByteSizeLong());
            //if(!res.SerializeToArray(buf.data(), static_cast<int>(buf.size()))) {
            //    throw std::runtime_error("failed to serialize handshake response");
            //}
            //
            //if(!m_curr_pipe_endpoint->try_to_write_for(std::move(buf), std::chrono::seconds(10))) {
            //    throw std::runtime_error("failed to write handshake response");
            //}
            //
            //auto pipe = std::move(m_curr_pipe_endpoint);
            //m_curr_pipe_endpoint.reset();
            //return std::make_unique<transport>(m_thread_pool,
            //                                   std::move(pipe),
            //                                   std::move(start_callback),
            //                                   std::move(stop_callback));

            return nullptr;
        } catch(...) {
            m_curr_pipe_endpoint.reset();
            throw;
        }
    }

    void server_connector::interrupt()
    {
        std::lock_guard guard(m_mtx);
        if(m_curr_pipe_endpoint) {
            m_curr_pipe_endpoint->invalidate();
        }
    }
}
