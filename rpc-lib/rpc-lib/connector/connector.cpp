#include "connector.h"
#include "rpc-lib/transport/transport.h"
#include "rpc-lib/authenticator/iauthenticator.h"
#include "rpc-lib/pipe/ipipe-env.h"
#include "rpc-lib/pipe/ipipe.h"

#pragma warning(push, 0)
#include "rpc-lib/authenticator/proto/auth.pb.h"
#pragma warning(pop)

namespace vshalygin::rpc {
    connector::connector(std::shared_ptr<ipipe_env> pipe_env,
                         std::shared_ptr<iauthenticator> authenticator)
        : m_pipe_env(std::move(pipe_env))
        , m_authenticator(std::move(authenticator))
    {
        assert(m_pipe_env);
        assert(m_authenticator);
    }

    std::unique_ptr<itransport> connector::create_transport()
    {
        auto pipe = m_pipe_env->open_pipe();
        if(!pipe->wait_connect_for(std::chrono::seconds(10))) {
            throw std::runtime_error("failed to wait pipe connection");
        }

        proto::auth_request req = m_authenticator->create_request();
        cl::buffer buff(req.ByteSizeLong());
        req.SerializeToArray(buff.data(), static_cast<int>(buff.size()));
        if(!pipe->try_to_write_for(std::move(buff), std::chrono::seconds(10))) {
            throw std::runtime_error("failed to send a connection message to server");
        }

        auto raw_res = pipe->try_to_read_for(std::chrono::seconds(10));
        if(!raw_res) {
            throw std::runtime_error("failed to read answer in specified time");
        }

        proto::auth_response res;
        if(!res.ParseFromArray(raw_res->data(), static_cast<int>(raw_res->size()))) {
            throw std::runtime_error("failed to parse answer");
        }

        if(!res.is_accepted()) {
            throw std::runtime_error("connect is not allowed by server side");
        }

        return std::make_unique<transport>(std::move(pipe));
    }
}
