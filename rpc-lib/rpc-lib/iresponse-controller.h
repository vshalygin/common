#pragma once
#include <cstdint>

namespace google::protobuf {
    class RpcController;
}

namespace vshalygin::rpc {
    class iresponse_controller
    {
    public:
        virtual ~iresponse_controller() = default;

        virtual uint64_t get_connection_id() const = 0;
    };

    iresponse_controller *to_response_controller(google::protobuf::RpcController *);
}
