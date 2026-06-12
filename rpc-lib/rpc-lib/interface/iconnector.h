#pragma once
#include "itransport.h"
#include <memory>

namespace vshalygin::rpc {
    //create_transport должен бросать исключение, если не удалось создать транспрот

    class iconnector
    {
    public:
        virtual ~iconnector() = default;

        virtual std::unique_ptr<itransport> create_transport() = 0;
    };
}
