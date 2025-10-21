#pragma once

namespace vsh::example {
    class irpc_server_connection
    {
    public:
        virtual ~irpc_server_connection() = default;

        virtual void listen() = 0;
        //TODO accept 
        virtual void close() = 0;
    };
}
