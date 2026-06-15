#pragma once
#include "rpc-lib/listener/ilistener.h"
#include "rpc-lib/connection/iconnection.h"
#include "rpc-lib/service/iservice.h"
#include "rpc-lib/channel/ichannel.h"
#include "rpc-lib/types/connection-state.h"
#include "rpc-lib/types/request-exception.h"
#include "rpc-lib/types/constants.h"
#include "rpc-lib/channel/request-callback/request-callback.h"

#include <common-lib/syncronization/guarded-value/guarded-value.h>
#include <common-lib/syncronization/latch/latch.h>
#include <common-lib/thread-pool/thread-pool.h>

#include <unordered_map>
#include <vector>
#include <utility>
#include <memory>
#include <functional>
#include <atomic>

namespace vshalygin::rpc {
    class server_endpoint final
    {
        using connection_change_state_handler_t = std::function<void(uint64_t, connection_state)>;
        using listener_change_state_handler_t = std::function<void(listener_state)>;

        template<typename Response>
        using request_callback_t = std::function<void(uint64_t, request_result, std::unique_ptr<Response>)>;

    public:
        template<typename Response>
        using response_results_t = std::vector<std::pair<request_result, std::unique_ptr<Response>>>;

        server_endpoint() = default;

        server_endpoint(std::unique_ptr<ilistener> listener,
                        std::shared_ptr<iservice> service,
                        std::shared_ptr<cl::thread_pool> thread_pool,
                        connection_change_state_handler_t &&connection_change_state_handler);

        server_endpoint(server_endpoint &) = delete;
        server_endpoint &operator=(server_endpoint &) = delete;

        server_endpoint(server_endpoint &&) = default;
        server_endpoint &operator=(server_endpoint &&) = default;

        void start_listen();
        bool is_listening() const;
        void stop_listen();

        void set_listener_change_state_handler(listener_change_state_handler_t &&handler);

        void drop_connection(uint64_t id);
        void drop_all_connections();

        size_t get_inactive_connections_count() const;
        size_t get_channels_count() const;

        template<typename Request, typename Response>
        std::unique_ptr<Response> make_request(uint64_t connection_id,
                                               const Request &req,
                                               const auto &create_stub,
                                               auto method);

        template<typename Request, typename Response>
        response_results_t<Response> make_request_all(const Request &req,
                                                      const auto &create_stub,
                                                      auto method);

        template<typename Request, typename Response>
        void make_request_async(uint64_t connection_id,
                                const Request &req,
                                const auto &create_stub,
                                auto method,
                                request_callback_t<Response> &&req_callback);

        template<typename Request, typename Response>
        size_t make_request_all_async(const Request &req,
                                      const auto &create_stub,
                                      auto method,
                                      request_callback_t<Response> &&req_callback);

    private:
        class impl;
        std::shared_ptr<impl> m_impl;
    };

    class server_endpoint::impl final
        : public std::enable_shared_from_this<impl>
    {
        class creator
        {};

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
        inline static std::shared_ptr<impl> create(std::unique_ptr<ilistener> listener,
                                                   std::shared_ptr<iservice> service,
                                                   std::shared_ptr<cl::thread_pool> thread_pool,
                                                   connection_change_state_handler_t &&connection_change_state_handler)
        {
            return std::make_shared<impl>(std::move(listener),
                                          std::move(service),
                                          std::move(thread_pool),
                                          std::move(connection_change_state_handler),
                                          creator());
        }

        impl(std::unique_ptr<ilistener> listener,
             std::shared_ptr<iservice> service,
             std::shared_ptr<cl::thread_pool> thread_pool,
             connection_change_state_handler_t &&connection_change_state_handler,
             creator);

        impl(impl &) = delete;
        impl &operator=(impl &) = delete;

        void start_listen();
        bool is_listening() const;
        void stop_listen();

        void set_listener_change_state_handler(listener_change_state_handler_t &&handler);

        void drop_connection(uint64_t id);
        void drop_all_connections();

        void set_new_connection_handler();

        size_t get_inactive_connections_count() const;
        size_t get_channels_count() const;

        template<typename Request, typename Response>
        std::unique_ptr<Response> make_request(uint64_t connection_id,
                                               const Request &req,
                                               const auto &create_stub,
                                               auto method)
        {
            auto channel = find_channel_or_throw(connection_id);
            auto latch = std::make_shared<cl::latch>(1);
            auto result_data = std::make_shared<req_result_data<Response>>();
            auto req_callback = create_sync_response_callback(result_data, latch);
            make_request_async<Request, Response>(connection_id,
                                                  std::move(channel),
                                                  req,
                                                  create_stub,
                                                  method,
                                                  req_callback);

            if(!latch->wait_for(RequestTimeout * 2)) {
                throw request_exception(request_result::unknown_error,
                                        "waiting request callback failed");
            }

            if(is_success(result_data->req_result)) {
                return std::move(result_data->response);
            }

            throw request_exception(result_data->req_result, "request failed");
        }

        template<typename Request, typename Response>
        response_results_t<Response> make_request_all(const Request &req,
                                                      const auto &create_stub,
                                                      auto method)
        {
            auto channels_with_id = get_all_channels_with_id();
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
                                                      create_stub,
                                                      method,
                                                      req_callback);
            }

            if(!latch->wait_for(RequestTimeout * 2)) {
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

        template<typename Request, typename Response>
        void make_request_async(uint64_t connection_id,
                                const Request &req,
                                const auto &create_stub,
                                auto method,
                                const request_callback_t<Response> &req_callback)
        {
            auto channel = find_channel_or_throw(connection_id);
            make_request_async<Request, Response>(connection_id,
                                                  std::move(channel),
                                                  req,
                                                  create_stub,
                                                  method,
                                                  req_callback);
        }

        template<typename Request, typename Response>
        size_t make_request_all_async(const Request &req,
                                      const auto &create_stub,
                                      auto method,
                                      const request_callback_t<Response> &req_callback)
        {
            auto channels_with_id = get_all_channels_with_id();
            for(auto &channel_with_id : channels_with_id) {
                make_request_async<Request, Response>(channel_with_id.first,
                                                      channel_with_id.second,
                                                      req,
                                                      create_stub,
                                                      method,
                                                      req_callback);
            }

            return channels_with_id.size();
        }

    private:
        template<typename Request, typename Response>
        void make_request_async(uint64_t connection_id,
                                std::shared_ptr<ichannel> channel,
                                const Request &req,
                                const auto &create_stub,
                                auto method,
                                const request_callback_t<Response> &req_callback)
        {
            assert(channel->get_connection());

            auto req_callback_with_id = [connection_id, req_callback]
                                        (request_result rc, std::unique_ptr<Response> response_msg) {
                req_callback(connection_id, rc, std::move(response_msg));
            };

            auto callback = request_callback<Response>::create_on_heap(std::move(req_callback_with_id));

            (create_stub(channel.get()).*method)(callback,
                                                 &req,
                                                 callback->get_response_ptr(),
                                                 callback);
        }

        template<typename Response>
        request_callback_t<Response> create_sync_response_callback
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

        void handle_new_connection(std::unique_ptr<itransport> transport,
                                   std::weak_ptr<impl> self);

        std::function<void(connection_state)>
            create_connection_change_state_handler(std::weak_ptr<impl> self, uint64_t connection_id) const;

        std::shared_ptr<ichannel> find_channel_or_throw(uint64_t connection_id) const;
        std::vector<std::pair<uint64_t, std::shared_ptr<ichannel>>> get_all_channels_with_id() const;

        void call_connection_state_change_handler(uint64_t connection_id, connection_state state);

    private:
        std::atomic_uint64_t m_next_connection_id = 0;

        std::unique_ptr<ilistener> m_listener;
        std::shared_ptr<iservice> m_service;

        std::shared_ptr<cl::thread_pool> m_thread_pool;

        using channel_map_t = std::unordered_map<uint64_t, std::shared_ptr<ichannel>>;
        using connection_map_t = std::unordered_map<uint64_t, std::shared_ptr<iconnection>>;
        cl::guarded_value<channel_map_t> m_channel_map;
        cl::guarded_value<connection_map_t> m_inactive_connection_map;

        connection_change_state_handler_t m_connection_change_state_handler;
    };

    template<typename Request, typename Response>
    std::unique_ptr<Response> server_endpoint::make_request(uint64_t connection_id,
                                                            const Request &req,
                                                            const auto &create_stub,
                                                            auto method)
    {
        return m_impl->make_request<Request, Response>(connection_id, req, create_stub, method);
    }

    template<typename Request, typename Response>
    server_endpoint::response_results_t<Response> server_endpoint::make_request_all(const Request &req,
                                                                                    const auto &create_stub,
                                                                                    auto method)
    {
        return m_impl->make_request_all<Request, Response>(req, create_stub, method);
    }

    template<typename Request, typename Response>
    void server_endpoint::make_request_async(uint64_t connection_id,
                                             const Request &req,
                                             const auto &create_stub,
                                             auto method,
                                             request_callback_t<Response> &&req_callback)
    {
        m_impl->make_request_async<Request, Response>(connection_id,
                                                      req,
                                                      create_stub,
                                                      method,
                                                      std::move(req_callback));
    }

    template<typename Request, typename Response>
    size_t server_endpoint::make_request_all_async(const Request &req,
                                                   const auto &create_stub,
                                                   auto method,
                                                   request_callback_t<Response> &&req_callback)
    {
        return m_impl->make_request_all_async<Request, Response>(req,
                                                                 create_stub,
                                                                 method,
                                                                 std::move(req_callback));
    }
}
