#pragma once
#include "rpc-lib/types/listener-state.h"
#include <functional>
#include <memory>

namespace vshalygin::rpc {
    class iconnection;
    class ichannel;

    //TODO write requesties
    //1) connect_handler может и не быть, нужно это учитывать
    //2) Все методы синхронные
    //3) set_connect_handler гарантированно вызывается до start, доп. синхранизация не нужна
    //4) set_change_state_handler может быть вызван в любой момент
    //5) change_state_handler должен гарантированно вызываться один после другого

    class ilistener
    {
    public:
        using change_state_handler_t = std::function<void(listener_state)>;

        virtual ~ilistener() = default;

        virtual void start() = 0;
        virtual void stop() = 0;
        virtual bool is_stopped() const = 0;

        virtual std::shared_ptr<ichannel> get_channel(uint64_t id) const = 0;

        using channels = std::vector<std::pair<uint64_t, std::shared_ptr<ichannel>>>;
        virtual channels get_all_channels() const = 0;

        virtual void set_change_state_handler(change_state_handler_t &&handler) = 0;

        virtual void drop_connection(uint64_t id);
        virtual void drop_all_connections();
    };
}
