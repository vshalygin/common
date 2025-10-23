#pragma once

namespace vsh::example {
    class irpc_client_connection
    {
    public:
        virtual ~irpc_client_connection() = default;

        virtual bool is_connected() const = 0;

        virtual int connect() = 0;
        virtual int close() = 0;
    };
}