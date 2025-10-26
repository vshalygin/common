#pragma once
#include "rpc-lib/common/listener/irecv-event-processor.h"

#include <common-lib/utils/guarded-value/guarded-value.h>
#include <common-lib/utils/buffer/buffer.h>

#include <memory>

namespace vsh::rpc {
    class server_event_processor final
        : public irecv_event_processor
    {
        using callback_type = std::function<void(const common_lib::buffer &)>;
        using guarded_cb_map = common_lib::guarded_value<std::unordered_map<uint64_t, callback_type>>;

    public:
        explicit server_event_processor(std::shared_ptr<guarded_cb_map> cb_map);

        server_event_processor(server_event_processor &) = delete;
        server_event_processor &operator=(server_event_processor) = delete;

        int process(const common_lib::buffer &buffer) override;

    private:
        int process_res(const common_lib::buffer &buffer);

    private:
        std::shared_ptr<guarded_cb_map> cb_map_;
    };
}
