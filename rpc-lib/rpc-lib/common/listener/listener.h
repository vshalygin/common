#pragma once
#include "ilistener.h"
#include "irecv-handler.h"
#include "rpc-lib/common/transport/itransport.h"
#include <common-lib/thread-pool/ithread-pool.h>

#include <memory>
#include <thread>

namespace vsh::rpc {
    class listener final
        : public ilistener
    {
    public:
        listener(std::shared_ptr<irecv_handler> recv_handler,
                 std::shared_ptr<itransport> transport,
                 std::shared_ptr<cl::ithread_pool> thread_pool);

        listener(listener &) = delete;
        listener &operator=(listener &) = delete;

        void start() override;

    private:
        bool listen();

    private:
        std::shared_ptr<irecv_handler> recv_handler_;
        std::shared_ptr<itransport> transport_;
        std::shared_ptr<cl::ithread_pool> thread_pool_;

        std::jthread listen_thread_;
    };
}
