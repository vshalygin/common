#pragma once
#include "iservice.h"

#include <rpc-lib/internal/controller/response-controller.h>
#include <rpc-lib/internal/transfer-message/transfer-message.h>

#pragma warning(push, 0)
#include <google/protobuf/message.h>
#pragma warning(pop)

#include <cassert>

namespace vshalygin::rpc::internal {
    template<typename Service>
    class service final
        : public iservice
    {
        using Message = google::protobuf::Message;
        using MessageFactory = google::protobuf::MessageFactory;

    public:
        explicit service(std::shared_ptr<Service> gservice,
                         std::shared_ptr<cl::thread_pool> thread_pool,
                         uint64_t connection_id);

        service(service &) = delete;
        service &operator=(service &) = delete;

        future<cl::buffer> process_request_async(cl::buffer &&request_message) override;

    private:
        static bool parse_proto_message(std::shared_ptr<Message> msg, cl::cbuffer_view buffer);

        static std::shared_ptr<Message> create_request_message(std::shared_ptr<Service> gservice,
                                                               uint32_t method_idx);

        static std::shared_ptr<Message> create_response_message(std::shared_ptr<Service> gservice,
                                                                uint32_t method_idx);

        static uint32_t get_methods_count(std::shared_ptr<Service> gservice);

    private:
        std::shared_ptr<Service> m_gservice;
        std::shared_ptr<cl::thread_pool> m_thread_pool;

        const uint64_t m_connection_id;
    };

    template<typename Service>
    service<Service>::service(std::shared_ptr<Service> gservice,
                              std::shared_ptr<cl::thread_pool> thread_pool,
                              uint64_t connection_id)
        : m_gservice(std::move(gservice))
        , m_thread_pool(std::move(thread_pool))
        , m_connection_id(connection_id)
    {}

    template<typename Service>
    future<cl::buffer> service<Service>::process_request_async(cl::buffer &&request_message)
    {
        assert(get_transfer_msg_type(request_message) == transfer_msg_type::req);
        auto promise = make_promise(m_thread_pool.get(), [](cl::buffer &&b) {
            return std::move(b);
        });
        auto future = promise.get_future();

        m_thread_pool->post([gservice = m_gservice,
                             connection_id = m_connection_id,
                             req_msg = std::move(request_message),
                             promise = std::move(promise)]() mutable
        {
            const auto message_number = get_msg_number_req(req_msg);
            const auto method_idx = get_msg_method_idx_req(req_msg);
            const auto serialized_req_message = get_serialized_proto_message(req_msg);

            if(!gservice || method_idx >= get_methods_count(gservice)) {
                auto raw_response = create_transfer_msg_res(message_number,
                                                            response_result::not_implemented,
                                                            nullptr);
                promise.resolve(std::move(raw_response));
                return;
            }

            auto req = create_request_message(gservice, method_idx);
            auto res = create_response_message(gservice, method_idx);

            if(!parse_proto_message(req, serialized_req_message)) {
                auto raw_response = create_transfer_msg_res(message_number,
                                                            response_result::request_parse_error,
                                                            res.get());
                promise.resolve(std::move(raw_response));
                return;
            }

            auto callback = [promise = std::move(promise),
                req, res, message_number](response_result rc) mutable {
                if(is_transfer_msg_too_big(res.get())) {
                    res->Clear();
                    rc = response_result::response_too_big;
                }

                promise.resolve(create_transfer_msg_res(message_number, rc, res.get()));
            };

            auto controller =
                response_controller<decltype(callback)>::create_on_heap(std::move(callback), connection_id);

            gservice->CallMethod(gservice->descriptor()->method(method_idx),
                                 controller,
                                 req.get(),
                                 res.get(),
                                 controller);
        });

        return future;
    }

    template<typename Service>
    bool service<Service>::parse_proto_message(std::shared_ptr<Message> msg, cl::cbuffer_view buffer)
    {
        return msg->ParseFromArray(buffer.data(), static_cast<int>(buffer.size()));
    }

    template<typename Service>
    std::shared_ptr<google::protobuf::Message>
        service<Service>::create_request_message(std::shared_ptr<Service> gservice,
                                                 uint32_t method_idx)
    {
        const auto req_desc = gservice->descriptor()->method(method_idx)->input_type();
        std::unique_ptr<Message> req
                          (MessageFactory::generated_factory()->GetPrototype(req_desc)->New());
        return req;
    }

    template<typename Service>
    std::shared_ptr<google::protobuf::Message>
        service<Service>::create_response_message(std::shared_ptr<Service> gservice,
                                                  uint32_t method_idx)
    {
        const auto res_desc = gservice->descriptor()->method(method_idx)->output_type();
        std::unique_ptr<Message> res
        (MessageFactory::generated_factory()->GetPrototype(res_desc)->New());
        return res;
    }

    template<typename Service>
    uint32_t service<Service>::get_methods_count(std::shared_ptr<Service> gservice)
    {
        return static_cast<uint32_t>(gservice->descriptor()->method_count());
    }
}
