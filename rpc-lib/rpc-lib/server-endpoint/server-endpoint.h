#pragma once
#include "rpc-lib/listener/listener.h"
#include "rpc-lib/connection/connection.h"
#include "rpc-lib/channel/channel.h"
#include "rpc-lib/service/service.h"
#include "rpc-lib/types/connection-state.h"
#include "rpc-lib/types/request-exception.h"
#include "rpc-lib/channel/request-callback/request-callback.h"

#include <common-lib/synchronization/guarded-value/guarded-value.h>
#include <common-lib/synchronization/latch/latch.h>
#include <common-lib/thread/thread-pool/thread-pool.h>

#include <unordered_map>
#include <vector>
#include <utility>
#include <memory>
#include <functional>

namespace vshalygin::rpc {
    template<typename GServiceStub>
    class server_endpoint final
    {
        using connection_change_callback_t =
            std::function<void(uint64_t, connection_state)>;

        template<typename Response>
        using request_callback_t =
            std::function<void(uint64_t, request_result, std::unique_ptr<Response>)>;

        template<typename Response>
        struct req_result_data
        {
            req_result_data()
                : req_result(request_result::unknown_error)
            {}

            uint64_t connection_id;
            request_result req_result;
            std::unique_ptr<Response> response;
        };

    public:
        template<typename Response>
        using response_results_t =
            std::vector<std::pair<request_result, std::unique_ptr<Response>>>;

        template<typename GService>
        server_endpoint(std::unique_ptr<GService> gservice,
                        std::shared_ptr<cl::thread_pool> thread_pool,
                        std::shared_ptr<iconnector> connector,
                        connection_change_callback_t &&connect_change_callback,
                        const std::chrono::milliseconds &request_timeout);

        server_endpoint(server_endpoint &) = delete;
        server_endpoint &operator=(server_endpoint &) = delete;

        void start_listen();
        bool is_listening() const;
        void stop_listen();

        void drop_connection(uint64_t id);
        void drop_all_connections();

        template<typename Request, typename Response>
        std::unique_ptr<Response> make_request(uint64_t connection_id,
                                               const Request &req,
                                               auto method);

        template<typename Request, typename Response>
        response_results_t<Response> make_request_all(const Request &req,
                                                      auto method);

        template<typename Request, typename Response>
        void make_request_async(uint64_t connection_id,
                                const Request &req,
                                auto method,
                                const request_callback_t<Response> &req_callback);

        template<typename Request, typename Response>
        size_t make_request_all_async(const Request &req,
                                      auto method,
                                      const request_callback_t<Response> &req_callback);

    private:
        template<typename Request, typename Response>
        void make_request_async(uint64_t connection_id,
                                std::shared_ptr<ichannel> channel,
                                const Request &req,
                                auto method,
                                const request_callback_t<Response> &req_callback);

        template<typename Response>
        request_callback_t<Response> create_sync_response_callback
            (std::shared_ptr<req_result_data<Response>> result_data,
             std::shared_ptr<cl::latch> latch);

    private:
        const std::chrono::milliseconds m_request_timeout;

        listener m_listener;
        std::shared_ptr<iservice> m_service;

        std::shared_ptr<cl::thread_pool> m_thread_pool;

        connection_change_callback_t m_connect_change_callback;
    };

    template<typename GServiceStub>
    template<typename GService>
    server_endpoint<GServiceStub>::server_endpoint
                                    (std::unique_ptr<GService> gservice,
                                     std::shared_ptr<cl::thread_pool> thread_pool,
                                     std::shared_ptr<iconnector> connector,
                                     connection_change_callback_t &&connect_change_callback,
                                     const std::chrono::milliseconds &request_timeout)
        : m_request_timeout(request_timeout)
        , m_listener(std::move(connector),
                     std::make_shared<service>(std::move(gservice)),
                     thread_pool,
                     std::move(connect_change_callback),
                     request_timeout)
        , m_service(std::make_shared<service>(std::move(gservice)))
        , m_thread_pool(thread_pool)
        , m_connect_change_callback(std::move(connect_change_callback))
    {}

    template<typename GServiceStub>
    void server_endpoint<GServiceStub>::start_listen()
    {
        m_listener->start();
    }

    template<typename GServiceStub>
    bool server_endpoint<GServiceStub>::is_listening() const
    {
        return !m_listener->is_stopped();
    }

    template<typename GServiceStub>
    void server_endpoint<GServiceStub>::stop_listen()
    {
        return m_listener->stop();
    }

    template<typename GServiceStub>
    void server_endpoint<GServiceStub>::drop_connection(uint64_t id)
    {
        m_listener->drop_connection(id);
    }

    template<typename GServiceStub>
    void server_endpoint<GServiceStub>::drop_all_connections()
    {
        m_listener->drop_all_connections();
    }

    template<typename GServiceStub>
    template<typename Request, typename Response>
    std::unique_ptr<Response> server_endpoint<GServiceStub>::make_request
                                                        (uint64_t connection_id,
                                                         const Request &req,
                                                         auto method)
    {
        auto channel = m_listener->get_channel(connection_id);
        auto latch = std::make_shared<cl::latch>(1);
        auto result_data = std::make_shared<req_result_data<Response>>();
        auto req_callback = create_sync_response_callback(result_data, latch);
        make_request_async<Request, Response>(connection_id,
                                              std::move(channel),
                                              req,
                                              method,
                                              req_callback);

        if(!latch->wait_for(m_request_timeout * 2)) {
            throw request_exception(request_result::unknown_error,
                                    "waiting request callback failed");
        }

        if(is_success(result_data->req_result)) {
            return std::move(result_data->response);
        }

        throw request_exception(result_data->req_result, "request failed");
    }

    template<typename GServiceStub>
    template<typename Request, typename Response>
    server_endpoint<GServiceStub>::response_results_t<Response>
        server_endpoint<GServiceStub>::make_request_all(const Request &req,
                                                        auto method)
    {
        auto channels_with_id = m_listener->get_all_channels();
        std::vector<std::shared_ptr<req_result_data<Response>>> result_data_v(channels_with_id.size());
        for(size_t i = 0; i < result_data_v.size(); ++i) {
            result_data_v[i] = std::make_shared<req_result_data<Response>>();
        }
        auto latch = std::make_shared<cl::latch>(channels_with_id.size());

        for(size_t i = 0; i < channels_with_id.size(); ++i) {
            auto req_callback = create_sync_response_callback(result_data_v[i], latch);
            make_request_async<Request, Response>(channels_with_id[i].first,
                                                  channels_with_id[i].second,
                                                  req,
                                                  method,
                                                  req_callback);
        }

        if(!latch->wait_for(m_request_timeout * 2)) {
            throw request_exception(request_result::unknown_error,
                                    "waiting request callbacks failed");
        }

        response_results_t<Response> ans(result_data_v.size());
        for(size_t i = 0; i < result_data_v.size(); ++i) {
            auto req_result = result_data_v[i]->req_result;
            auto &response = result_data_v[i]->response;
            ans[i] = std::pair<request_result, std::unique_ptr<Response>>
                (req_result, std::move(response));
        }

        return ans;
    }

    template<typename GServiceStub>
    template<typename Request, typename Response>
    void server_endpoint<GServiceStub>::make_request_async
                                            (uint64_t connection_id,
                                             const Request &req,
                                             auto method,
                                             const request_callback_t<Response> &req_callback)
    {
        auto channel = m_listener->get_channel(connection_id);
        make_request_async<Request, Response>(connection_id,
                                              std::move(channel),
                                              req,
                                              method,
                                              req_callback);
    }

    template<typename GServiceStub>
    template<typename Request, typename Response>
    size_t server_endpoint<GServiceStub>::make_request_all_async
                                              (const Request &req,
                                               auto method,
                                               const request_callback_t<Response> &req_callback)
    {
        auto channels_with_id = m_listener->get_all_channels();
        for(auto &channel_with_id : channels_with_id) {
            make_request_async<Request, Response>(channel_with_id.first,
                                                  channel_with_id.second,
                                                  req,
                                                  method,
                                                  req_callback);
        }

        return channels_with_id.size();
    }

    template<typename GServiceStub>
    template<typename Request, typename Response>
    void server_endpoint<GServiceStub>::make_request_async
                                            (uint64_t connection_id,
                                             std::shared_ptr<ichannel> channel,
                                             const Request &req,
                                             auto method,
                                             const request_callback_t<Response> &req_callback)
    {
        assert(channel->get_connection());

        auto req_callback_with_id = [connection_id, req_callback]
        (request_result rc, std::unique_ptr<Response> response_msg) {
            req_callback(connection_id, rc, std::move(response_msg));
        };

        auto callback = request_callback<Response>::create_on_heap(std::move(req_callback_with_id));

        (GServiceStub(channel.get()).*method)(callback,
                                              &req,
                                              callback->get_response_ptr(),
                                              callback);
    }

    template<typename GServiceStub>
    template<typename Response>
    server_endpoint<GServiceStub>::request_callback_t<Response>
        server_endpoint<GServiceStub>::create_sync_response_callback
                                  (std::shared_ptr<req_result_data<Response>> result_data,
                                   std::shared_ptr<cl::latch> latch)
    {
        return [result_data = std::move(result_data), latch = std::move(latch)]
        (uint64_t connection_id, request_result rc, std::unique_ptr<Response> response) {
             result_data->connection_id = connection_id;
             result_data->req_result = rc;
             result_data->response = std::move(response);
             latch->count_down();
        };
    }
}
