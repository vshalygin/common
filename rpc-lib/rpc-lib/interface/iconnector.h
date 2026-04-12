#pragma once
#include "itransport.h"
#include <memory>

namespace vshalygin::rpc {
    class iconnector
    {
    public:
        virtual ~iconnector() = default;

        virtual std::unique_ptr<itransport> create_transport() = 0;
    };
}
