#pragma once
#include <memory>

namespace vshalygin::rpc {
    class itransport;
    //create_transport должен бросать исключение, если не удалось создать транспрот

    class iconnector
    {
    public:
        virtual ~iconnector() = default;

        virtual std::unique_ptr<itransport> create_transport() = 0;
    };
}
