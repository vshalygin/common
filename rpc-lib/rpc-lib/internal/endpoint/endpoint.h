#pragma once
#include <rpc-lib/types/future.h>
#include <rpc-lib/internal/connection/iconnection.h>
#include <rpc-lib/internal/channel/channel.h>
#include <rpc-lib/internal/controller/request-controller.h>

#include <common-lib/thread/thread-pool/thread-pool.h>
#include <common-lib/thread/thread-pool/thread-pool-task.h>

#include <memory>

namespace vshalygin::rpc::internal {
    template<typename GServiceStub>
    class endpoint final
    {
    public:
        template<typename Response>
        using request_future = future<ftuple<request_result, std::unique_ptr<Response>>>;

        explicit endpoint(std::unique_ptr<iconnection> connection,
                          std::shared_ptr<cl::thread_pool> thread_pool);

        endpoint(const endpoint &) = delete;
        endpoint &operator=(const endpoint &) = delete;

        template<typename Request, typename Response>
        request_future<Response> make_request(auto stub_method,
                                              const Request &req);

        void start();

        void disconnect();
        bool is_connected() const;
        void set_disconnect_callback(cl::thread_pool_task<void()> &&callback);

    private:
        std::shared_ptr<cl::thread_pool> m_thread_pool;

        channel m_channel;
        GServiceStub m_service_stub;
    };

    template<typename GServiceStub>
    endpoint<GServiceStub>::endpoint(std::unique_ptr<iconnection> connection,
                                     std::shared_ptr<cl::thread_pool> thread_pool)
        : m_thread_pool(std::move(thread_pool))
        , m_channel(std::move(connection))
        , m_service_stub(&m_channel)
    {}

    template<typename GServiceStub>
    template<typename Request, typename Response>
    endpoint<GServiceStub>::request_future<Response>
        endpoint<GServiceStub>::make_request(auto stub_method, const Request &req)
    {
        auto promise = make_promise(m_thread_pool.get(), [](request_result r, std::unique_ptr<Response> m) {
            return ftuple{ r, std::move(m) };
        });
        auto future = promise.get_future();

        auto response = std::make_unique<Response>();
        auto response_ptr = response.get();

        auto controller = request_controller<Response>::create_on_heap(std::move(promise),
                                                                       std::move(response));

        (m_service_stub.*stub_method)(controller,
                                      &req,
                                      response_ptr,
                                      controller);

        return future;
    }

    template<typename GServiceStub>
    void endpoint<GServiceStub>::start()
    {
        m_channel.start();
    }

    template<typename GServiceStub>
    void endpoint<GServiceStub>::disconnect()
    {
        m_channel.disconnect();
    }

    template<typename GServiceStub>
    bool endpoint<GServiceStub>::is_connected() const
    {
        return m_channel.is_connected();
    }

    template<typename GServiceStub>
    void endpoint<GServiceStub>::set_disconnect_callback(cl::thread_pool_task<void()> &&callback)
    {
        m_channel.set_disconnect_callback(std::move(callback));
    }
}
