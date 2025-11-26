#pragma once
#include <common-lib/utils/buffer/buffer.h>

#pragma warning(push, 0)
#include <google/protobuf/service.h>
#pragma warning(pop)

namespace vsh::rpc {
    class itransfer_entry_creator
    {
    public:
        virtual ~itransfer_entry_creator() = default;

        virtual cl::buffer create_entry(const ::google::protobuf::MethodDescriptor *method,
                                        const ::google::protobuf::Message *request,
                                        uint64_t &transfer_entry_number) = 0;

    };
}
