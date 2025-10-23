#pragma once

namespace vsh::rpc {
    class iserver_connection
    {
    public:
        virtual ~iserver_connection() = default;

        virtual void listen() = 0;
        //TODO accept 
        virtual void close() = 0;
    };
}
