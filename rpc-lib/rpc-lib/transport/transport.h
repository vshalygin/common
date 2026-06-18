#pragma once
#include "itransport.h"
#include <atomic>

namespace vshalygin::rpc {
    class ipipe;

    class transport
        : public itransport
    {
    public:
        explicit transport(std::shared_ptr<ipipe> pipe,
                           std::function<void()> &&stop_callback);

        transport(transport &) = delete;
        transport &operator=(transport &) = delete;

        ~transport() override;

        void send_async(cl::buffer &&message,
                        std::function<void()> &&error_handler) override;
        void recv_async(std::function<void(bool, cl::buffer &&)> &&handler) override;

        void stop() override;
        bool is_running() const override;

    private:
        std::shared_ptr<ipipe> m_pipe;

        std::function<void()> m_stop_callback;

        std::atomic_bool m_is_stopped = false;
    };
}
