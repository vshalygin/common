#pragma once
#include "rpc-lib/channel/channel.h"
#include "rpc-lib/service/service.h"
#include "rpc-lib/connection/connection.h"
#include "rpc-lib/connector/client-connector.h"
#include "rpc-lib/channel/request-callback/request-callback.h"
#include "rpc-lib/types/request-exception.h"
#include "rpc-lib/types/connection-state.h"

#include <common-lib/syncronization/guarded-value/guarded-value.h>
#include <common-lib/syncronization/event/event.h>
#include <common-lib/thread-pool/thread-pool.h>

#include <functional>
#include <memory>
#include <chrono>

namespace vshalygin::rpc {
    class iauthenticator;
    class ipipe_env;

    template<typename GServiceStub>
    class client_endpoint final
    {
        using state_change_callback_t = std::function<void(connection_state)>;

        template<typename Response>
        using request_callback_t = std::function<void(request_result, std::unique_ptr<Response>)>;

        template<typename Response>
        struct response_data
        {
            request_result result = request_result::unknown_error;
            std::unique_ptr<Response> response;
        };

    public:
        template<typename GService>
        client_endpoint(std::unique_ptr<GService> gservice,
                        std::shared_ptr<cl::thread_pool> thread_pool,
                        std::shared_ptr<iauthenticator> authenticator,
                        std::shared_ptr<ipipe_env> pipe_env,
                        state_change_callback_t &&handler,
                        const std::chrono::milliseconds &request_timeout);

        client_endpoint(client_endpoint &) = delete;
        client_endpoint &operator=(client_endpoint &) = delete;

        void connect();
        void disconnect();
        bool is_connected() const;

        template<typename Request, typename Response>
        std::unique_ptr<Response> make_request(const Request &req,
                                               auto method);

        template<typename Request, typename Response>
        void make_request_async(const Request &req,
                                auto method,
                                request_callback_t<Response> &&req_callback);

    private:
        std::shared_ptr<iservice> m_service;
        std::unique_ptr<ichannel> m_channel;
        std::shared_ptr<iconnector> m_connector;
        std::shared_ptr<cl::thread_pool> m_thread_pool;
        std::shared_ptr<state_change_callback_t> m_change_callback;
        const std::chrono::milliseconds m_request_timeout;

        GServiceStub m_service_stub;
    };

    template<typename GServiceStub>
    template<typename GService>
    client_endpoint<GServiceStub>::client_endpoint(std::unique_ptr<GService> gservice,
                                                   std::shared_ptr<cl::thread_pool> thread_pool,
                                                   std::shared_ptr<iauthenticator> authenticator,
                                                   std::shared_ptr<ipipe_env> pipe_env,
                                                   state_change_callback_t &&handler,
                                                   const std::chrono::milliseconds &req_timeout)
        : m_service(std::make_shared<service>(std::move(gservice)))
        , m_channel(std::make_unique<channel>())
        , m_connector(std::make_shared<client_connector>(thread_pool, authenticator, pipe_env))
        , m_thread_pool(thread_pool)
        , m_change_callback(std::make_shared<state_change_callback_t>)
        , m_request_timeout(req_timeout)
        , m_service_stub(m_channel.get())
    {}

    template<typename GServiceStub>
    void client_endpoint<GServiceStub>::connect()
    {
        auto connection0 = connection::create(m_thread_pool,
                                              m_change_callback,
                                              m_connector,
                                              m_service,
                                              m_request_timeout);
        connection0->activate();
        m_channel->set_connection(connection0);
    }

    template<typename GServiceStub>
    void client_endpoint<GServiceStub>::disconnect()
    {
        m_channel->get_connection()->deactivate(); //TODO продумать
    }

    template<typename GServiceStub>
    bool client_endpoint<GServiceStub>::is_connected() const
    {
        return m_channel->get_connection()->is_active();
    }

    template<typename GServiceStub>
    template<typename Request, typename Response>
    std::unique_ptr<Response> client_endpoint<GServiceStub>::make_request(const Request &req,
                                                                          auto method)
    {
        struct sync_req_data
        {
            response_data<Response> data;
            cl::event sync_event;
        };
        auto sync_data = std::make_shared<sync_req_data>();

        auto req_callback =
            [sync_data] (request_result rc, std::unique_ptr<Response> response) {
                sync_data->data.result = rc;
                sync_data->data.response = std::move(response);
                sync_data->sync_event.set();
            };

        auto callback = request_callback<Response>::create_on_heap(std::move(req_callback));

        (m_service_stub.*method)(callback,
                                 &req,
                                 callback->get_response_ptr(),
                                 callback);

        if(!sync_data->sync_event.wait_for(m_request_timeout * 2)) {
            throw request_exception(request_result::unknown_error,
                                    "waiting request callback failed");
        }

        if(is_fail(sync_data->data.result)) {
            throw request_exception(sync_data->data.result, "request failed");
        }

        return std::move(sync_data->data.response);
    }

    template<typename GServiceStub>
    template<typename Request, typename Response>
    void client_endpoint<GServiceStub>::make_request_async
       (const Request &req,
        auto method,
        request_callback_t<Response> &&req_callback)
    {
        auto callback = request_callback<Response>::create_on_heap(std::move(req_callback));

        (m_service_stub.*method)(callback,
                                 &req,
                                 callback->get_response_ptr(),
                                 callback);
    }
}
