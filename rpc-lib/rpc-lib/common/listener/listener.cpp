#include "listener.h"

namespace vsh::rpc {
    listener::listener(std::shared_ptr<irecv_handler> recv_handler,
                       std::shared_ptr<itransport> transport,
                       std::shared_ptr<common_lib::ithread_pool> thread_pool)
        : recv_handler_(std::move(recv_handler))
        , transport_(std::move(transport))
        , thread_pool_(std::move(thread_pool))
    {}

    void listener::start()
    {
        auto task = [this](std::stop_token st) {
            while(!st.stop_requested() && transport_->is_active()) {
                listen();
            }
        };

        listen_thread_ = std::jthread(std::move(task));
    }

    bool listener::listen()
    {
        common_lib::buffer buffer;
        transport_->recv(buffer); //TODO check fail

        auto task = [buffer = std::move(buffer), recv_handler = recv_handler_]() {
            recv_handler->process(buffer);
        };

        thread_pool_->post(std::move(task));

        return true;
    }

}
