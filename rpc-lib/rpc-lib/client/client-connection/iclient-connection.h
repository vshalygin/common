#pragma once

namespace vsh::rpc {
    class iclient_connection
    {
    public:
        virtual ~iclient_connection() = default;

        virtual bool is_connected() const = 0;

        virtual int connect() = 0; //TODO return error code
        virtual int close() = 0;
    };
}