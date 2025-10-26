#include "client-channel.h"

#include "rpc-lib/common/transfer-entry/transfer-entry.h"

namespace vsh::rpc {
    client_channel::client_channel(std::shared_ptr<itransport> transport,
                                   std::shared_ptr<ithread_pool> thread_pool,
                                   std::shared_ptr<guarded_cb_map> cb_map,
                                   const std::string &client_id)
        : transport_(std::move(transport))
        , thread_pool_(std::move(thread_pool))
        , cb_map_(std::move(cb_map))
        , client_id_(client_id)
    {}

    void client_channel::CallMethod(const MethodDescriptor *method,
                                    RpcController * /*controller*/,
                                    const Message *request,
                                    Message *response,
                                    Closure *done)
    {
        assert(method);
        assert(request);
        assert(response);
        assert(done);

        auto transfer_entry_number = counter_.fetch_add(1);
        auto method_idx = static_cast<unsigned>(method->index());
        auto transfer_entry = create_transfer_entry_req(static_cast<unsigned>(transfer_entry_number),
                                                        client_id_,
                                                        method_idx,
                                                        request);

        auto callback = [this, response, done](const common_lib::buffer &answer_entry)
        {
            auto serialized_message = get_serialized_message_res(answer_entry);
            response->ParseFromArray(serialized_message.data(), static_cast<int>(serialized_message.size()));

            done->Run();
        };

        auto task = [cb_map = cb_map_, callback = std::move(callback),
                     transfer_entry_number, transport = transport_,
                     transfer_entry = std::move(transfer_entry)]() mutable
        {
            {
                auto [guard, m] = cb_map->get();
                m.insert({ static_cast<unsigned>(transfer_entry_number), std::move(callback) });
            }

            transport->send(std::move(transfer_entry));
        };

        thread_pool_->post(std::move(task));
    }
}
