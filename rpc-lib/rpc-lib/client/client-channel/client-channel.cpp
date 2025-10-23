#include "client-channel.h"
#include "client/client-transport/iclient-transport.h"

#include <common-lib/thread-pool/ithread-pool.h>

#pragma warning(push, 0)
#include <google/protobuf/message.h>
#pragma warning(pop)

namespace vsh::rpc {
    client_channel::client_channel(std::shared_ptr<iclient_transport> transport,
                                   std::shared_ptr<ithread_pool> thread_pool)
        : transport_(std::move(transport))
        , thread_pool_(std::move(thread_pool))
    {
        listen_thread_ = std::jthread([this](std::stop_token st) {
                                          while(!st.stop_requested()) {
                                              listen_server();
                                          }
                                      });
    }

    client_channel::~client_channel() = default;

    void client_channel::CallMethod(const MethodDescriptor * /*method*/,
                                    RpcController * /*controller*/,
                                    const Message *request,
                                    Message *response,
                                    Closure *done)
    {
        thread_pool_->post([this, response, done, req = request->SerializeAsString()]() {
                               callback_ = [done, response](const std::string &res){
                                   response->ParseFromString(res);
                                   done->Run();
                               };
                               transport_->send(req);
                           });
    }

    void client_channel::listen_server()
    {
        std::string ans;
        transport_->recv(ans);
        thread_pool_->post([this, ans = std::move(ans)]() {
                               if(callback_) {
                                   callback_(ans);
                               }
                           });
    }
}
