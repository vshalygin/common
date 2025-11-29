#pragma once
#include "iconnection.h"
#include "itransport.h"

namespace vsh::rpc {
    class connection
        : public iconnection
    {
    public:
        explicit connection(std::unique_ptr<itransport> transport);

        connection(connection &) = delete;
        connection &operator=(connection &) = delete;

        ~connection();

        void request_async(cl::buffer &&message,
                           std::function<void(request_result, cl::buffer &&)> &&handler) override;

        void set_response_async_processor(
            std::function<void(cl::buffer &&, response_result_callback &&)> &&processor) override;

        void set_disconnect_handler(std::function<void()> &&handler) override;
        bool is_connected() const override;

    private:
        class impl;
        std::shared_ptr<impl> m_impl;
    };
}
