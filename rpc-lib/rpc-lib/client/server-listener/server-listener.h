#pragma once
#include "iserver-listener.h"

#include "rpc-lib/client/client-transport/iclient-transport.h"

#include <common-lib/utils/guarded-value/guarded-value.h>
#include <common-lib/utils/buffer/buffer.h>
#include <common-lib/thread-pool/ithread-pool.h>

#include <functional>
#include <unordered_map>
#include <memory>
#include <thread>

namespace vsh::rpc {
    class server_listener final
        : public iserver_listener
    {
        using callback_type = std::function<void(const common_lib::buffer &)>;
        using guarded_cb_map = common_lib::guarded_value<std::unordered_map<unsigned, callback_type>>;

        using ithread_pool = common_lib::ithread_pool;

    public:
        server_listener(std::shared_ptr<guarded_cb_map> cb_map,
                        std::shared_ptr<iclient_transport> transport,
                        std::shared_ptr<ithread_pool> thread_pool);

        server_listener(server_listener &) = delete;
        server_listener &operator=(server_listener &) = delete;

        void start() override;

    private:
        bool listen();

    private:
        std::shared_ptr<guarded_cb_map> cb_map_;
        std::shared_ptr<iclient_transport> transport_;
        std::shared_ptr<ithread_pool> thread_pool_;

        std::jthread listen_thread_;
    };
}
