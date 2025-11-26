#include "transfer-entry-req-creator.h"

#include "rpc-lib/common/transfer-entry/transfer-entry.h"

namespace vsh::rpc {
    transfer_entry_req_creator::transfer_entry_req_creator(const std::string &client_id)
        : m_client_id(client_id)
    {}


    cl::buffer transfer_entry_req_creator::create_entry(const MethodDescriptor *method,
                                                        const Message *request,
                                                        uint64_t &transfer_entry_number)
    {
        transfer_entry_number = m_counter.fetch_add(1);
        auto method_idx = static_cast<unsigned>(method->index());

        return create_transfer_entry_req(transfer_entry_number,
                                         m_client_id,
                                         method_idx,
                                         request);
    }
}
