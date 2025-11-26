#include "channel.h"

#include "rpc-lib/common/transfer-entry/transfer-entry.h"

namespace vsh::rpc {
    channel::channel(std::shared_ptr<itransport> transport,
                     std::shared_ptr<ithread_pool> thread_pool,
                     std::shared_ptr<guarded_cb_map> cb_map,
                     std::unique_ptr<itransfer_entry_creator> entry_creator)
        : transport_(std::move(transport))
        , thread_pool_(std::move(thread_pool))
        , cb_map_(std::move(cb_map))
        , entry_creator_(std::move(entry_creator))
    {}

    void channel::CallMethod(const MethodDescriptor *method,
                             RpcController * /*controller*/,
                             const Message *request,
                             Message *response,
                             Closure *done)
    {
        assert(method);
        assert(request);
        assert(response);
        assert(done);

        uint64_t transfer_entry_number = 0;
        auto transfer_entry = entry_creator_->create_entry(method, request, transfer_entry_number);

        auto callback = [this, response, done](const cl::buffer &answer_entry)
        {
            auto serialized_message = get_serialized_message(answer_entry);
            response->ParseFromArray(serialized_message.data(), static_cast<int>(serialized_message.size()));

            done->Run();
        };

        auto task = [cb_map = cb_map_, callback = std::move(callback),
                     transfer_entry_number, transport = transport_,
                     transfer_entry = std::move(transfer_entry)]() mutable
        {
            {
                auto [guard, m] = cb_map->get();
                m.insert({ transfer_entry_number, std::move(callback) });
            }

            transport->send(std::move(transfer_entry));
        };

        thread_pool_->post(std::move(task));
    }
}
