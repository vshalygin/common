#pragma once
#include "rpc-lib/interface/itransport.h"
#include "common-lib/pipe/pipe.h"

#include <mutex>

namespace vshalygin::rpc {
    class pipe_transport
        : public itransport
    {
    public:
        explicit pipe_transport(std::shared_ptr<cl::pipe> pipe);

        pipe_transport(pipe_transport &) = delete;
        pipe_transport &operator=(pipe_transport &) = delete;

        void send_async(cl::buffer &&message,
                        std::function<void()> &&error_handler) const override;
        void recv_async(std::function<void(cl::buffer &&)> &&handler) const override;

        void start(std::function<void()> &&start_callback, std::function<void()> &&stop_callback) override;
        void stop() override;
        bool is_stopped() const override;

    private:
        std::shared_ptr<cl::pipe> pipe_;

        std::function<void()> stop_callback_;

        std::mutex mtx_;
        bool is_started_ = false;
    };
}
