#pragma once
#include "rpc-lib/types/listener-state.h"
#include <functional>
#include <memory>

namespace vsh::rpc {
    class itransport;

    //TODO write requesties
    class ilistener
    {
    public:
        using connect_handler_t = std::function<void(std::unique_ptr<itransport>)>;
        using change_state_handler_t = std::function<void(listener_state)>;

        virtual ~ilistener() = default;

        virtual void start() = 0;
        virtual void stop() = 0;
        virtual bool is_stopped() const = 0;
        virtual void set_change_state_handler(change_state_handler_t &&handler) = 0;

        virtual void set_connect_handler(connect_handler_t &&handler) = 0;
    };
}
