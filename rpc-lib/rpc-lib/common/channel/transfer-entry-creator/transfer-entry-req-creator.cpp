#include "transfer-entry-req-creator.h"

#include "rpc-lib/common/transfer-entry/transfer-entry.h"

namespace vsh::rpc {
    transfer_entry_req_creator::transfer_entry_req_creator(const std::string &client_id)
        : client_id_(client_id)
    {}


    common_lib::buffer transfer_entry_req_creator::create_entry(const MethodDescriptor *method,
                                                                const Message *request,
                                                                uint64_t &transfer_entry_number)
    {
        transfer_entry_number = counter_.fetch_add(1);
        auto method_idx = static_cast<unsigned>(method->index());

        return create_transfer_entry_req(transfer_entry_number,
                                         client_id_,
                                         method_idx,
                                         request);
    }
}
