#pragma once
#pragma warning(push, 0)
#include <google/protobuf/service.h>
#pragma warning(pop)

#include <cassert>

namespace vshalygin::rpc {
    class null_rpc_controller
        : public google::protobuf::RpcController
    {
    public:
        void Reset() override
        {}

        bool Failed() const override
        {
            return false;
        }

        std::string ErrorText() const override
        {
            return "";
        }

        void StartCancel() override
        {}

        void SetFailed(const std::string & /*reason*/) override
        {}

        bool IsCanceled() const override
        {
            return false;
        }

        void NotifyOnCancel(google::protobuf::Closure * /*callback*/) override
        {}
    };
}
