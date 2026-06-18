#include "client-connector.h"
#include "rpc-lib/pipe/ipipe.h"
#include "rpc-lib/pipe/ipipe-env.h"
#include "rpc-lib/authenticator/iauthenticator.h"
#include "rpc-lib/transport/transport.h"
#include "rpc-lib/types/interrupt-exception.h"

#pragma warning(push, 0)
#include "rpc-lib/authenticator/proto/auth.pb.h"
#pragma warning(pop)

#include <chrono>
#include <stdexcept>

namespace vshalygin::rpc {
    client_connector::client_connector(std::shared_ptr<iauthenticator> authenticator,
                                       std::shared_ptr<ipipe_env> pipe_env)
        : m_authenticator(std::move(authenticator))
        , m_pipe_env(std::move(pipe_env))
    {}

    std::unique_ptr<transport>
        client_connector::create_transport(std::function<void()> &&stop_callback) const
    {
        std::unique_lock guard(m_mtx);
        assert(!m_curr_pipe);

        try {
            m_curr_pipe = m_pipe_env->open_pipe();

            guard.unlock();
            auto wait_res = m_curr_pipe->wait_connect_for(std::chrono::seconds(10));
            guard.lock();

            if(is_fail(wait_res)) {
                throw std::runtime_error("failed to wait pipe connection: " + to_string(wait_res));
            }

            proto::auth_request req = m_authenticator->create_request();
            cl::buffer buff(req.ByteSizeLong());
            req.SerializeToArray(buff.data(), static_cast<int>(buff.size()));
            if(!m_curr_pipe->try_to_write_for(std::move(buff), std::chrono::seconds(10))) {
                throw std::runtime_error("failed to send a handshake message to server");
            }

            auto raw_res = m_curr_pipe->try_to_read_for(std::chrono::seconds(10));
            if(!raw_res) {
                throw std::runtime_error("failed to read handshake answer in specified time");
            }

            proto::auth_response res;
            if(!res.ParseFromArray(raw_res->data(), static_cast<int>(raw_res->size()))) {
                throw std::runtime_error("failed to parse handshake answer");
            }

            if(!res.is_accepted()) {
                throw std::runtime_error("connect is not allowed by server side");
            }

            auto pipe = std::move(m_curr_pipe);
            m_curr_pipe.reset();
            return std::make_unique<transport>(std::move(pipe), std::move(stop_callback));
        } catch (...) {
            m_curr_pipe.reset();
            throw;
        }
        
    }

    void client_connector::interrupt()
    {
        std::unique_lock guard(m_mtx);
        if(m_curr_pipe) {
            m_curr_pipe->invalidate();
        }
    }
}
