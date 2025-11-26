#include "listener.h"

namespace vsh::rpc {
    listener::listener(std::shared_ptr<irecv_handler> recv_handler,
                       std::shared_ptr<itransport> transport,
                       std::shared_ptr<cl::ithread_pool> thread_pool)
        : m_recv_handler(std::move(recv_handler))
        , m_transport(std::move(transport))
        , m_thread_pool(std::move(thread_pool))
    {}

    void listener::start()
    {
        auto task = [this](std::stop_token st) {
            while(!st.stop_requested() && m_transport->is_active()) {
                listen();
            }
        };

        m_listen_thread = std::jthread(std::move(task));
    }

    bool listener::listen()
    {
        cl::buffer buffer;
        m_transport->recv(buffer); //TODO check fail

        auto task = [buffer = std::move(buffer), recv_handler = m_recv_handler]() {
            recv_handler->process(buffer);
        };

        m_thread_pool->post(std::move(task));

        return true;
    }

}
