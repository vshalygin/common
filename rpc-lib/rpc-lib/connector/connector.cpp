#include "connector.h"
#include "rpc-lib/transport/transport.h"
#include "rpc-lib/pipe/ipipe-env.h"
#include "rpc-lib/pipe/ipipe.h"

#pragma warning(push, 0)
#include "rpc-lib/transport/proto/pipe-auth.pb.h"
#pragma warning(pop)

namespace vshalygin::rpc {
    connector::connector(std::shared_ptr<ipipe_env> pipe_env,
                         const std::string &listener_pipe_name)
        : pipe_env_(std::move(pipe_env))
        , listener_pipe_name_(listener_pipe_name)
    {
        assert(pipe_env_);
    }

    std::unique_ptr<itransport> connector::create_transport()
    {
        auto listener_pipe = pipe_env_->open_pipe(listener_pipe_name_);
        if(!listener_pipe->wait_connect_for(std::chrono::seconds(10))) {
            throw std::runtime_error("failed to wait listener pipe connection");
        }

        proto::pipe_auth_request req;
        req.set_auth_data("trust me, i'm Doctor"); //other side trusts unconditionally
        cl::buffer buff(req.ByteSizeLong());
        req.SerializeToArray(buff.data(), static_cast<int>(buff.size()));
        if(!listener_pipe->try_to_write_for(std::move(buff), std::chrono::seconds(10))) {
            throw std::runtime_error("failed to send a connection message to listener");
        }

        auto raw_res = listener_pipe->try_to_read_for(std::chrono::seconds(10));
        if(!raw_res) {
            throw std::runtime_error("failed to read answer in specified time");
        }

        proto::pipe_auth_response res;
        if(!res.ParseFromArray(raw_res->data(), static_cast<int>(raw_res->size()))) {
            throw std::runtime_error("failed to parse answer");
        }

        if(!res.is_accepted()) {
            throw std::runtime_error("authentication is not allowed by server side");
        }

        auto pipe = pipe_env_->open_pipe(res.pipe_name());
        if(!pipe->wait_connect_for(std::chrono::seconds(10))) {
            throw std::runtime_error("failed to wait worker pipe connection");
        }

        return std::make_unique<transport>(std::move(pipe));
    }
}
