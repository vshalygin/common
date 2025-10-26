#pragma once
namespace vsh::rpc {
    class ilistener
    {
    public:
        virtual ~ilistener() = default;

        virtual void start() = 0;
    };
}
