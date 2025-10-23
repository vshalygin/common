#pragma once
#include <string>

namespace vsh::rpc {
    class iclient_transport
    {
    public:
        virtual ~iclient_transport() = default;

        virtual int send(const std::string &buff) = 0;
        virtual int send(std::string &&buff) = 0;

        virtual int recv(std::string &buff) = 0;
    };
}
