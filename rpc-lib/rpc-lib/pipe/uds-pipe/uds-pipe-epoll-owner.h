#pragma once
#ifdef __linux__

#include <linux-lib/types/epoll.h>
#include <memory>

namespace vshalygin::rpc::internal {
    class uds_pipe_epoll_owner final
        : public std::enable_shared_from_this<uds_pipe_epoll_owner>
    {
        uds_pipe_epoll_owner();

    public:
        static std::shared_ptr<uds_pipe_epoll_owner> create();
        
        uds_pipe_epoll_owner(const uds_pipe_epoll_owner &) = delete;
        uds_pipe_epoll_owner &operator=(const uds_pipe_epoll_owner &) = delete;

        

    private:
        std::shared_ptr<linux::epoll> m_epoll;
    };
}

#endif
