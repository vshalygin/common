#pragma once
#include <common-lib/utils/buffer.h>
#include <memory>
#include <functional>

namespace vshalygin::rpc {
    class iservice
    {
    public:
        virtual ~iservice() = default;

        virtual void process_request(cl::buffer &&request_message,
                                     std::function<void(cl::buffer &&)> &&raw_response_callback) = 0;
    };
}
