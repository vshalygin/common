#pragma once
#include "request-controller-base.h"

#include <rpc-lib/types/request-result.h>
#include <rpc-lib/types/future.h>

#include <common-lib/mpl/type-transform.h>

namespace vshalygin::rpc::internal {
    template<typename Response>
    class request_controller final
        : public request_controller_base
        , public google::protobuf::Closure
    {
        using promise_t = promise<ftuple<request_result, std::unique_ptr<Response>>(
                                  request_result, std::unique_ptr<Response>)>;

        request_controller(promise_t &&promise,
                           std::unique_ptr<Response> &&response);

    public:
        static request_controller *create_on_heap(promise_t &&promise,
                                                  std::unique_ptr<Response> &&response);

        request_controller(request_controller &) = delete;
        request_controller &operator=(request_controller &) = delete;

        void Run() override;

        void set_result(request_result r) override;

    private:
        request_result m_request_result = request_result::unknown_error;

        std::unique_ptr<Response> m_response;
        promise_t m_promise;

        const int m_uncaught_exceptions;
    };

    template<typename Response>
    request_controller<Response> *request_controller<Response>::create_on_heap(
        promise_t &&promise, std::unique_ptr<Response> &&response)
    {
        return new request_controller(std::move(promise), std::move(response));
    }

    template<typename Response>
    request_controller<Response>::request_controller(
        promise_t &&promise, std::unique_ptr<Response> &&response)
        : m_response(std::move(response))
        , m_promise(std::move(promise))
        , m_uncaught_exceptions(std::uncaught_exceptions())
    {
        assert(m_response);
    }

    template<typename Response>
    void request_controller<Response>::Run()
    {
        try {
            if(std::uncaught_exceptions() > m_uncaught_exceptions) {
                m_request_result = request_result::unknown_error;
            }

            m_promise.resolve(m_request_result, std::move(m_response));
        } catch(...) {
        }

        delete this;
    }

    template<typename Response>
    void request_controller<Response>::set_result(request_result r)
    {
        m_request_result = r;
    }
}
