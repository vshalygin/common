#pragma once
#include "rpc-lib/types/response-result.h"

#pragma warning(push, 0)
#include <google/protobuf/service.h>
#pragma warning(pop)

#include <functional>
#include <memory>

namespace vsh::rpc {
    class response_callback
        : public google::protobuf::RpcController
        , public google::protobuf::Closure
    {
        using callback_t = std::function<void(response_result)>;

        response_callback(callback_t &&callback);

    public:
        response_callback(response_callback &) = delete;
        response_callback &operator=(response_callback &) = delete;

        static response_callback *create_on_heap(callback_t &&callback);

        void Run() override;

        void Reset() override;
        bool Failed() const override;
        std::string ErrorText() const override;
        void StartCancel() override;
        void SetFailed(const std::string &reason) override;
        bool IsCanceled() const override;
        void NotifyOnCancel(Closure * /*callback*/) override;

    private:
        response_result m_response_result = response_result::ok;
        callback_t m_callback;
    };
}
