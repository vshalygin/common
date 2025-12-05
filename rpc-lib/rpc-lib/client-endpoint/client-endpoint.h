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

        template<typename Response>
        struct req_result_data
        {
            request_result req_result = request_result::unknown_error;
            std::unique_ptr<Response> response;
        };

    public:
        client_endpoint(std::shared_ptr<iservice> service,
                        std::unique_ptr<ichannel> channel,
                        std::shared_ptr<iconnection> connection,
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
            auto result_data = std::make_shared<req_result_data<Response>>();
            auto sync_event = std::make_shared<cl::event>();

            auto req_callback = [result_data, sync_event]
                                (request_result rc, std::unique_ptr<Response> response) {
                result_data->req_result = rc;
                result_data->response = std::move(response);
                sync_event->set();
            };

            auto callback = request_callback<Response>::create_on_heap(std::move(req_callback));

            (service_stub.*method)(callback,
                                   &req,
                                   callback->get_response_ptr(),
                                   callback);

            if(!sync_event->wait_for(RequestTimeout * 2)) {
                throw request_exception(request_result::unknown_error,
                                        "waiting request callback failed");
            }

            if(is_fail(result_data->req_result)) {
                throw request_exception(result_data->req_result, "request failed");
            }

            return std::move(result_data->response);
        }

        template<typename Request, typename Response>
        void make_request_async(const Request &req,
                                auto &service_stub,
                                auto method,
                                request_callback_t<Response> &&req_callback)
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
