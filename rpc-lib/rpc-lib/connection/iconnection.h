#pragma once
//TODO delete this interface

#include "rpc-lib/types/request-result.h"
#include <common-lib/utils/buffer/buffer.h>
#include <functional>

namespace vshalygin::rpc {
    class iconnection
    {
    public:
        using request_callback_t = std::function<void(request_result, cl::buffer &&)>;

        virtual ~iconnection() = default;

        virtual void activate() = 0;
        virtual void deactivate() = 0;
        virtual bool is_active() const = 0;

        virtual void request_async(cl::buffer &&message,
                                   request_callback_t &&callback) = 0;
    };
}
