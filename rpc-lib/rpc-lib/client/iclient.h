#pragma once

namespace vsh::rpc {
    class iclient
    {
    public:
        virtual ~iclient() = default;

        virtual int connect() = 0; //TODO return error code
        virtual int disconnect() = 0;
    };
}
