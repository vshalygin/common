#pragma once
#include <rpc-lib/internal/connection/iconnection.h>
#include <rpc-lib/internal/channel/channel.h>
#include <rpc-lib/internal/controller/request-controller.h>

#include <common-lib/thread/thread.h>

#include <memory>

namespace vshalygin::rpc::internal {
    template<typename RemoteStub>
    class endpoint final
    {
    public:
        template<typename Response>
        using request_future = cl::future<cl::thread_pool, cl::ftuple<request_result, std::unique_ptr<Response>>>;

        explicit endpoint(std::unique_ptr<iconnection> connection,
                          cl::thread_pool *thread_pool);

        endpoint(const endpoint &) = delete;
        endpoint &operator=(const endpoint &) = delete;

        template<typename Request, typename Response, typename StubMethod>
        request_future<Response> make_request(StubMethod stub_method,
                                              const Request &req);

        void start();

        void disconnect();
        bool is_connected() const;
        void set_disconnect_callback(cl::thread_pool_task<void()> &&callback);

    private:
        cl::thread_pool *m_thread_pool;

        channel m_channel;
        RemoteStub m_service_stub;
    };

    template<typename RemoteStub>
    endpoint<RemoteStub>::endpoint(std::unique_ptr<iconnection> connection,
                                     cl::thread_pool *thread_pool)
        : m_thread_pool(thread_pool)
        , m_channel(std::move(connection))
        , m_service_stub(&m_channel)
    {}

    template<typename RemoteStub>
    template<typename Request,typename Response, typename StubMethod>
    typename endpoint<RemoteStub>::template request_future<Response>
        endpoint<RemoteStub>::make_request(StubMethod stub_method, const Request &req)
    {
        cl::promise<cl::thread_pool,
                    cl::ftuple<request_result, std::unique_ptr<Response>>> promise(m_thread_pool);
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

    template<typename RemoteStub>
    void endpoint<RemoteStub>::start()
    {
        m_channel.start();
    }

    template<typename RemoteStub>
    void endpoint<RemoteStub>::disconnect()
    {
        m_channel.disconnect();
    }

    template<typename RemoteStub>
    bool endpoint<RemoteStub>::is_connected() const
    {
        return m_channel.is_connected();
    }

    template<typename RemoteStub>
    void endpoint<RemoteStub>::set_disconnect_callback(cl::thread_pool_task<void()> &&callback)
    {
        m_channel.set_disconnect_callback(std::move(callback));
    }
}
