#pragma once
#include <string>
#include <queue>
#include <mutex>
#include <condition_variable>

namespace vsh::example {
    class pseudopipe final
    {
    private:
        pseudopipe() = default;

    public:
        pseudopipe(pseudopipe &) = delete;
        pseudopipe &operator=(pseudopipe &) = delete;

        static pseudopipe &instance_cs(); //info from client to server;
        static pseudopipe &instance_sc(); //info from server to client;

        int send(const std::string &buff);
        int recv(std::string &buff);

    private:
        std::queue<std::string> queue_;

        std::mutex mtx_;
        std::condition_variable cv_;
    };
}
