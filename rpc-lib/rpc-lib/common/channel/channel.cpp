#include "channel.h"

#include "rpc-lib/common/transfer-entry/transfer-entry.h"

namespace vsh::rpc {
    channel::channel(std::shared_ptr<itransport> transport,
                     std::shared_ptr<ithread_pool> thread_pool,
                     std::shared_ptr<guarded_cb_map> cb_map,
                     std::unique_ptr<itransfer_entry_creator> entry_creator)
        : m_transport(std::move(transport))
        , m_thread_pool(std::move(thread_pool))
        , m_cb_map(std::move(cb_map))
        , m_entry_creator(std::move(entry_creator))
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
        auto transfer_entry = m_entry_creator->create_entry(method, request, transfer_entry_number);

        auto callback = [this, response, done](const cl::buffer &answer_entry)
        {
            auto serialized_message = get_serialized_message(answer_entry);
            response->ParseFromArray(serialized_message.data(), static_cast<int>(serialized_message.size()));

            done->Run();
        };

        auto task = [cb_map = m_cb_map, callback = std::move(callback),
                     transfer_entry_number, transport = m_transport,
                     transfer_entry = std::move(transfer_entry)]() mutable
        {
            {
                auto [guard, m] = cb_map->get();
                m.insert({ transfer_entry_number, std::move(callback) });
            }

            transport->send(std::move(transfer_entry));
        };

        m_thread_pool->post(std::move(task));
    }
}
