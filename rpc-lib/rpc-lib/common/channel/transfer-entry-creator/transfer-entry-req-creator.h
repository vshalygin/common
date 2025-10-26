#pragma once
#include "itransfer-entry-creator.h"

#include <string>
#include <atomic>

namespace vsh::rpc {
    class transfer_entry_req_creator
        : public itransfer_entry_creator
    {
        using MethodDescriptor = google::protobuf::MethodDescriptor;
        using RpcController = google::protobuf::RpcController;
        using Message = google::protobuf::Message;

    public:
        explicit transfer_entry_req_creator(const std::string &client_id);

        transfer_entry_req_creator(transfer_entry_req_creator &) = delete;
        transfer_entry_req_creator &operator=(transfer_entry_req_creator) = delete;

        common_lib::buffer create_entry(const MethodDescriptor *method,
                                        const Message *request,
                                        uint64_t &transfer_entry_number) override;

    private:
        const std::string client_id_;
        std::atomic_uint64_t counter_ = 0;
    };
}
