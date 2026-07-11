#pragma once
#include "endpoint.h"
#include <rpc-lib/types/connection-state.h>
#include <rpc-lib/types/future.h>
#include <rpc-lib/connector/server-connector.h>
#include <rpc-lib/pipe/iserver-pipe-env.h>
#include <rpc-lib/service/service.h>

#include <common-lib/thread/thread-pool/thread-pool.h>
#include <common-lib/synchronization/guarded-value/guarded-value.h>

#include <functional>
#include <memory>
#include <vector>
#include <utility>
#include <unordered_map>

namespace vshalygin::rpc {
    enum class server_endpoint_state
    {
        start_listening,
        stop_listening
    };

    template<typename GClientServiceStub, typename GServerService>
    class service_endpoint
    {
    public:
        template<typename Response>
        using request_future = future<ftuple<request_result, std::unique_ptr<Response>>>;

    private:
        class impl;
        std::shared_ptr<impl> m_impl;
    };

    template<typename GClientServiceStub, typename GServerService>
    class service_endpoint<GClientServiceStub, GServerService>::impl
        : public std::enable_shared_from_this<impl>
    {
    public:
        impl(std::function<void(server_endpoint_state)> &&on_state_change,
             std::function<void(uint64_t, connection_state)> &&on_connection_change,
             std::shared_ptr<cl::thread_pool> thread_pool);

        impl(const impl &) = delete;
        impl &operator=(const impl &) = delete;

        void start_listening();
        void stop_listening();
        bool is_listening() const;

        template<typename Request, typename Response, typename StubMethod>
        request_future<Response>
            make_request(uint64_t connection_id, StubMethod stub_method, const Request &req);

        template<typename Request, typename Response, typename StubMethod>
        std::vector<std::pair<uint64_t, request_future<Response>>>
            make_request_all(StubMethod stub_method, const Request &req);

        void set_connector(std::unique_ptr<server_connector> connector);

    private:
        std::shared_ptr<cl::thread_pool> m_thread_pool;

        std::function<void(server_endpoint_state)> m_on_state_change;
        std::function<void(uint64_t, connection_state)> m_on_connection_change;

        std::unique_ptr<server_connector> m_connector;

        cl::guarded_value<std::unordered_map<uint64_t, endpoint<GClientServiceStub>>> m_endpoints_map;
    };

    template<typename GClientServiceStub, typename GServerService>
    service_endpoint<GClientServiceStub, GServerService>::impl::impl(
        std::function<void(server_endpoint_state)> &&on_state_change,
        std::function<void(uint64_t, connection_state)> &&on_connection_change,
        std::shared_ptr<cl::thread_pool> thread_pool)
        : m_thread_pool(thread_pool)
        , m_on_state_change(std::move(on_state_change))
        , m_on_connection_change(std::move(on_connection_change))
    {}

    template<typename GClientServiceStub, typename GServerService>
    void service_endpoint<GClientServiceStub, GServerService>::impl::set_connector(
        std::unique_ptr<server_connector> connector)
    {
        m_connector = std::move(connector);
    }

    template<typename GClientServiceStub, typename GServerService>
    void start_listening();
    void stop_listening();
    bool is_listening() const;
}
