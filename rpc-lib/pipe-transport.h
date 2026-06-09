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

        void start() override;
        void stop() override;
        bool is_stopped() const override;

        void set_start_callback(std::function<void()> &&callback) override;
        void set_stop_callback(std::function<void()> &&callback) override;

    private:
        std::shared_ptr<cl::pipe> pipe_;

        std::function<void()> start_callback_;
        std::function<void()> stop_callback_;

        std::mutex start_mtx_;
        bool is_started_ = false;
    };
}
