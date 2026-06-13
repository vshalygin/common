#pragma once
#include "ilistener.h"
#include <common-lib/syncronization/guarded-value/guarded-value.h>
#include <thread>

namespace vshalygin::rpc {
    class ipipe_env;
    class ipipe;

    class listener
        : public ilistener
    {
    public:
        explicit listener(std::shared_ptr<ipipe_env> pipe_env,
                          const std::string &listener_pipe_name);

        listener(listener &) = delete;
        listener &operator=(listener &) = delete;

        void start() override;
        void stop() override;
        bool is_stopped() const override;
        void set_connect_handler(connect_handler_t &&handler) override;

        void set_change_state_handler(change_state_handler_t &&handler) override;

    private:
        void create_new_connection();

    private:
        std::shared_ptr<ipipe_env> pipe_env_;
        std::shared_ptr<ipipe> current_listener_pipe_;
        bool is_running_ = false;

        const std::string listener_pipe_name_;

        connect_handler_t connect_handler_;

        cl::guarded_value<change_state_handler_t> state_change_handler_;

        mutable std::mutex mtx_;
        std::jthread listen_thread_;
    };
}
