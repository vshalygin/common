#pragma once

namespace vsh::rpc {
    class iserver_listener
    {
    public:
        virtual ~iserver_listener() = default;

        virtual void start() = 0;
    };
}
