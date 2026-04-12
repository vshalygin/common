#pragma once
#pragma warning(push, 0)
#include <google/protobuf/service.h>
#pragma warning(pop)

#include <memory>

namespace vshalygin::rpc {
    class iconnection;

    class ichannel
        : public google::protobuf::RpcChannel
    {
    public:
        virtual ~ichannel() = default;

        virtual void set_connection(std::shared_ptr<iconnection> connection) = 0;
        virtual std::shared_ptr<iconnection> get_connection() const = 0;
        virtual void drop_connection() = 0;
    };
}
