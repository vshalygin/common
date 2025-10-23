#pragma once

namespace vsh::example {
    class irpc_server
    {
    public:
        virtual ~irpc_server() = default;

        virtual void run() = 0;
    };
}
