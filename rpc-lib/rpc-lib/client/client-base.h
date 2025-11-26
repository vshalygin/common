#pragma once
#include "iclient.h"

#include "rpc-lib/client/client-closure/client-closure.h"

#include <common-lib/utils/event/event.h>
#include <common-lib/utils/guarded-value/guarded-value.h>
#include <common-lib/utils/buffer/buffer.h>

#include <memory>
#include <string>
#include <functional>
#include <unordered_map>

namespace vsh::cl {
    class ithread_pool;
}

namespace google::protobuf {
    class RpcChannel;
}

namespace vsh::rpc {
    class iclient_connection;
    class itransport;
    class ilistener;

    class client_base
        : public iclient
    {
        using callback_type = std::function<void(const cl::buffer &)>;
        using guarded_cb_map = cl::guarded_value<std::unordered_map<uint64_t, callback_type>>;

    protected:
        client_base(std::shared_ptr<cl::ithread_pool> thread_pool,
                    std::shared_ptr<iclient_connection> connection,
                    std::shared_ptr<itransport> transport,
                    std::unique_ptr<ilistener> server_listener);

        client_base(std::shared_ptr<cl::ithread_pool> thread_pool,
                    std::shared_ptr<iclient_connection> connection,
                    std::shared_ptr<itransport> transport);

    public:
        client_base(client_base &) = delete;
        client_base &operator=(client_base &) = delete;

        int connect() override;
        int disconnect() override;

    protected:
        ::google::protobuf::RpcChannel *get_channel() const;

        template<typename Request, typename Response>
        std::unique_ptr<Response> call_method(const Request &req, auto &service_stub, auto method)
        {
            auto response = std::make_unique<Response>();
            auto response_ptr = response.get();

            cl::event sync_event;
            auto callback = [&sync_event]() {
                sync_event.set();
            };

            auto done = rpc::client_closure::create(std::move(callback));

            (service_stub.*method)(nullptr, //TODO add rpc_controller
                                   &req,
                                   response_ptr,
                                   done);

            sync_event.wait();

            return response;
        }

    private:
        const std::string m_client_id;

        std::shared_ptr<guarded_cb_map> m_cb_map;

        std::shared_ptr<cl::ithread_pool> m_thread_pool;
        std::shared_ptr<iclient_connection> m_connection;
        std::unique_ptr<::google::protobuf::RpcChannel> m_channel;
        std::unique_ptr<ilistener> m_server_listener;
    };
}
