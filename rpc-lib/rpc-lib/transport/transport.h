#pragma once
#include "itransport.h"
#include <mutex>

namespace vshalygin::rpc {
    class ipipe;

    class transport
        : public itransport
    {
    public:
        explicit transport(std::shared_ptr<ipipe> pipe);

        transport(transport &) = delete;
        transport &operator=(transport &) = delete;

        ~transport() override;

        void send_async(cl::buffer &&message,
                        std::function<void()> &&error_handler) override;
        void recv_async(std::function<void(bool, cl::buffer &&)> &&handler) override;

        void start(std::function<void()> &&start_callback, std::function<void()> &&stop_callback) override;
        void stop() override;
        bool is_running() const override;

    private:
        std::shared_ptr<ipipe> m_pipe;

        std::function<void()> m_stop_callback;

        mutable std::mutex m_mtx;

        enum class state
        {
            init,
            started,
            stopped
        } m_state = state::init;
    };
}
