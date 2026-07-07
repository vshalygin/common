#pragma once
#include "request-controller-base.h"
#include "rpc-lib/types/request-result.h"
#include "common-lib/mpl/type-transform.h"

#include <functional>

namespace vshalygin::rpc {
    template<typename Response, typename Callback>
    class request_controller final
        : public request_controller_base
        , public google::protobuf::Closure
    {
        request_controller(Callback &&callback,
                           std::unique_ptr<Response> &&response);

    public:
        static request_controller *create_on_heap(Callback &&callback,
                                                  std::unique_ptr<Response> &&response);

        request_controller(request_controller &) = delete;
        request_controller &operator=(request_controller &) = delete;

        void Run() override;

        void set_result(request_result r) override;

    private:
        request_result m_request_result = request_result::unknown_error;

        std::unique_ptr<Response> m_response;
        cl::remove_type_qualifiers_t<Callback> m_callback;

        const int m_uncaught_exceptions;
    };

    template<typename Response, typename Callback>
    request_controller<Response, Callback> *
        request_controller<Response, Callback>::create_on_heap(Callback &&callback,
                                                               std::unique_ptr<Response> &&response)
    {
        return new request_controller(std::forward<Callback>(callback), std::move(response));
    }

    template<typename Response, typename Callback>
    request_controller<Response, Callback>::request_controller(Callback &&callback,
                                                               std::unique_ptr<Response> &&response)
        : m_response(std::move(response))
        , m_callback(std::forward<Callback>(callback))
        , m_uncaught_exceptions(std::uncaught_exceptions())
    {
        assert(m_response);
    }

    template<typename Response, typename Callback>
    void request_controller<Response, Callback>::Run()
    {
        try {
            if(std::uncaught_exceptions() > m_uncaught_exceptions) {
                m_request_result = request_result::unknown_error;
            }
            m_callback(m_request_result, std::move(m_response));
        } catch(...) {
        }

        delete this;
    }

    template<typename Response, typename Callback>
    void request_controller<Response, Callback>::set_result(request_result r)
    {
        m_request_result = r;
    }
}
