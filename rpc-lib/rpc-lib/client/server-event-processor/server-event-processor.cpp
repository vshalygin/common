#include "server-event-processor.h"
#include "rpc-lib/common/transfer-entry/transfer-entry.h"

namespace vsh::rpc {
    server_event_processor::server_event_processor(std::shared_ptr<guarded_cb_map> cb_map)
        : cb_map_(std::move(cb_map))
    {}

    int server_event_processor::process(const common_lib::buffer &buffer)
    {
        auto entry_type = get_transfer_entry_type(buffer);
        switch(entry_type) {
            case transfer_type::res:
                return process_res(buffer);
            default:
                assert(!"server_event_process: unexpected type");
                return -1;
        }

    }

    int server_event_processor::process_res(const common_lib::buffer &buffer)
    {
        auto entry_number = get_entry_number_res(buffer);
        callback_type callback;
        {
            auto [guard, m] = cb_map_->get();
            auto it = m.find(entry_number);
            if(it != m.end()) {
                callback = std::move(it->second);
                m.erase(it);
            }
        }
        if(callback) {
            callback(buffer);
        }

        return 0;
    }
}
