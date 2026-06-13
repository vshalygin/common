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

        void send_async(cl::buffer &&message,
                        std::function<void()> &&error_handler) override;
        void recv_async(std::function<void(cl::buffer &&)> &&handler) override;

        void start(std::function<void()> &&start_callback, std::function<void()> &&stop_callback) override;
        void stop() override;
        bool is_running() const override;

    private:
        std::shared_ptr<ipipe> pipe_;

        std::function<void()> stop_callback_;

        mutable std::mutex mtx_;

        enum class state
        {
            init,
            started,
            stopped
        } state_ = state::init;
    };
}
