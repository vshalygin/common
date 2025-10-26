#pragma once
#include "ilistener.h"
#include "irecv-event-processor.h"
#include "rpc-lib/common/transport/itransport.h"
#include <common-lib/thread-pool/ithread-pool.h>

#include <memory>
#include <thread>

namespace vsh::rpc {
    class listener final
        : public ilistener
    {
    public:
        listener(std::shared_ptr<irecv_event_processor> event_processor,
                 std::shared_ptr<itransport> transport,
                 std::shared_ptr<common_lib::ithread_pool> thread_pool);

        listener(listener &) = delete;
        listener &operator=(listener &) = delete;

        void start() override;

    private:
        bool listen();

    private:
        std::shared_ptr<irecv_event_processor> event_processor_;
        std::shared_ptr<itransport> transport_;
        std::shared_ptr<common_lib::ithread_pool> thread_pool_;

        std::jthread listen_thread_;
    };
}
