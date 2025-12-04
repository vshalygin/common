#pragma once
#include "rpc-lib/channel/ichannel.h"
#include "rpc-lib/connection/iconnection.h"
#include "rpc-lib/service/iservice.h"
#include "rpc-lib/interface/iconnector.h"
#include "rpc-lib/channel/request-callback/request-callback.h"
#include "rpc-lib/types/constants.h"
#include "rpc-lib/types/request-exception.h"

#include <common-lib/syncronization/guarded-value/guarded-value.h>
#include <common-lib/syncronization/event/event.h>

#include <functional>
#include <memory>

namespace vsh::rpc {
    class client_endpoint final
    {
        template<typename Response>
        using request_callback_t = std::function<void(request_result, std::unique_ptr<Response>)>;

    public:
        client_endpoint(std::shared_ptr<iservice> service,
                        std::unique_ptr<ichannel> channel,
                        std::unique_ptr<iconnection> connection,
                        std::unique_ptr<iconnector> connector);

        client_endpoint(client_endpoint &) = delete;
        client_endpoint &operator=(client_endpoint &) = delete;

        void connect();
        void disconnect();
        bool is_connected() const;

        void set_connection_change_state_handler(std::function<void(connection_state)> &&handler);

        template<typename Request, typename Response>
        std::unique_ptr<Response> make_request(const Request &req, auto &service_stub, auto method)
        {
            struct req_result_data
            {
                req_result_data()
                    : req_result(request_result::ok)
                    , response(std::make_unique<Response>())
                {}

                cl::event sync_event;
                request_result req_result;
                std::unique_ptr<Response> response;
            };
            auto result_data = std::make_shared<req_result_data>();

            auto req_callback = [result_data](request_result rc, std::unique_ptr<Response> response) {
                result_data->req_result = rc;
                result_data->response = std::move(response);
                result_data->sync_event.set();
            };

            auto callback = request_callback<Response>::create_on_heap(std::move(req_callback));

            (service_stub.*method)(callback,
                                   &req,
                                   callback->get_response_ptr(),
                                   callback);

            if(!result_data->sync_event.wait_for(RequestTimeout * 2)) {
                throw request_exception(request_result::unknown_error,
                                        "waiting request callback failed");
            }

            if(is_success(result_data->req_result)) {
                return std::move(result_data->response);
            }

            throw request_exception(result_data->req_result, "request failed");
        }

        template<typename Request, typename Response>
        void make_request_async(const Request &req, auto &service_stub,
                                auto method, request_callback_t<Response> &&req_callback)
        {
            auto callback = request_callback<Response>::create_on_heap(std::move(req_callback));

            (service_stub.*method)(callback,
                                   &req,
                                   callback->get_response_ptr(),
                                   callback);
        }

    private:
        std::unique_ptr<ichannel> m_channel;
        std::shared_ptr<iconnection> m_connection;
        std::unique_ptr<iconnector> m_connector;
    };
}
