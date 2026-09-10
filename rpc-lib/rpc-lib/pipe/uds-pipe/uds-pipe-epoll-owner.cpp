#ifdef __linux__
#include "uds-pipe-epoll-owner.h"

namespace vshalygin::rpc::internal {
    std::shared_ptr<uds_pipe_epoll_owner> uds_pipe_epoll_owner::create()
    {
        return std::shared_ptr<uds_pipe_epoll_owner>(new uds_pipe_epoll_owner);
    }

    uds_pipe_epoll_owner::uds_pipe_epoll_owner()
        : m_epoll(std::make_shared<linux::epoll>())
    {}

}

#endif
