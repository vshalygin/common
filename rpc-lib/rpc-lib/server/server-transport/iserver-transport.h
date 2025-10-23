#pragma once
#include <string>

namespace vsh::rpc {
    class iserver_transport
    {
    public:
        virtual ~iserver_transport() = default;

        virtual int send(const std::string &buff) = 0;
        virtual int send(std::string &&buff) = 0;

        virtual int recv(std::string &buff) = 0;
    };
}
