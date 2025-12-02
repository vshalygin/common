#pragma once
#include "rpc-lib/types/request-result.h"

#pragma warning(push, 0)
#include <google/protobuf/service.h>
#pragma warning(pop)

#include <functional>

namespace vsh::rpc {
    template<typename Response>
    class request_callback
        : public google::protobuf::RpcController
        , public google::protobuf::Closure
    {
        using callback_t = std::function<void(request_result, std::unique_ptr<Response>)>;

        request_callback(callback_t &&callback)
            : m_response(std::make_unique<Response>())
            , m_callback(std::move(callback))
        {
            assert(m_response);
            assert(m_callback);
        }

    public:
        request_callback(request_callback &) = delete;
        request_callback &operator=(request_callback &) = delete;

        inline static request_callback *create_on_heap(callback_t &&callback)
        {
            return new request_callback(std::move(callback));
        }

        Response *get_response_ptr() const
        {
            return m_response.get();
        }

        void Run() override
        {
            try {
                m_callback(m_request_result, std::move(m_response));
            } catch(...) {
                //TODO safe log
            }

            delete this;
        }

        void Reset() override
        {
            assert(!"Reset function should never be called in current implementation");
        }

        bool Failed() const override
        {
            return is_fail(m_request_result);
        }

        std::string ErrorText() const override
        {
            return to_string(m_request_result);
        }

        void StartCancel() override
        {
            assert(!"StartCancel function should never be called in current implementation");
        }

        void SetFailed(const std::string &reason) override
        {
            assert(is_fail(request_result_from_string(reason)));
            m_request_result = request_result_from_string(reason);
        }

        bool IsCanceled() const override
        {
            return m_request_result == request_result::canceled;
        }

        void NotifyOnCancel(Closure * /*callback*/) override
        {
            assert(!"NotifyOnCancel function should never be called in current implementation");
        }

    private:
        request_result m_request_result = request_result::ok;

        std::unique_ptr<Response> m_response;
        std::function<void(request_result, std::unique_ptr<Response>)> m_callback;
    };
}
