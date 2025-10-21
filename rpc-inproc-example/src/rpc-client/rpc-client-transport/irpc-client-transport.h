#pragma once
#include <string>

namespace vsh::example {
    class irpc_client_transport
    {
    public:
        virtual ~irpc_client_transport() = default;

        virtual int send(const std::string &buff) = 0;
        virtual int send(std::string &&buff) = 0;

        virtual int recv(std::string &buff) = 0;
    };
}
