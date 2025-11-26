#include "client-recv-handler.h"
#include "rpc-lib/common/transfer-entry/transfer-entry.h"

namespace vsh::rpc {
    client_recv_handler::client_recv_handler(std::shared_ptr<guarded_cb_map> cb_map)
        : cb_map_(std::move(cb_map))
    {}

    int client_recv_handler::process(const cl::buffer &buffer)
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

    int client_recv_handler::process_res(const cl::buffer &buffer)
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
