#include "client-channel.h"
#include "rpc-lib/client/client-transport/iclient-transport.h"
#include "rpc-lib/common/transfer-entry/transfer-entry.h"

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

    void client_channel::CallMethod(const MethodDescriptor *method,
                                    RpcController * /*controller*/,
                                    const Message *request,
                                    Message *response,
                                    Closure *done)
    {
        auto req = create_transfer_entry_req(1,
                                             "E8C53F9F4A2246A1BDB4DCE68EC8379D",
                                             method->index(),
                                             request);
        thread_pool_->post([this, response, done, req = std::move(req)]() {
                               callback_ = [done, response](const common_lib::buffer &res){
                                   auto serialized_message = get_serialized_message_res({ res.data(), res.size() });
                                   response->ParseFromArray(serialized_message.data(),
                                                            static_cast<int>(serialized_message.size()));
                                   done->Run();
                               };
                               transport_->send(req);
                           });
    }

    void client_channel::listen_server()
    {
        common_lib::buffer ans;
        transport_->recv(ans);
        thread_pool_->post([this, ans = std::move(ans)]() {
                               if(callback_) {
                                   callback_(ans);
                               }
                           });
    }
}
