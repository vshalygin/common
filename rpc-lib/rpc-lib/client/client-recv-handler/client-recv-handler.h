#pragma once
#include "rpc-lib/common/listener/irecv-handler.h"

#include <common-lib/utils/guarded-value/guarded-value.h>
#include <common-lib/utils/buffer/buffer.h>

#include <memory>

namespace vsh::rpc {
    class client_recv_handler final
        : public irecv_handler
    {
        using callback_type = std::function<void(const cl::buffer &)>;
        using guarded_cb_map = cl::guarded_value<std::unordered_map<uint64_t, callback_type>>;

    public:
        explicit client_recv_handler(std::shared_ptr<guarded_cb_map> cb_map);

        client_recv_handler(client_recv_handler &) = delete;
        client_recv_handler &operator=(client_recv_handler) = delete;

        int process(const cl::buffer &buffer) override;

    private:
        int process_res(const cl::buffer &buffer);

    private:
        std::shared_ptr<guarded_cb_map> cb_map_;
    };
}
