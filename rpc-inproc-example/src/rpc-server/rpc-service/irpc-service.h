#pragma once

namespace vsh::example {
    class irpc_service
    {
    public:
        virtual ~irpc_service() = default;

        virtual void run() = 0;
    };
}
