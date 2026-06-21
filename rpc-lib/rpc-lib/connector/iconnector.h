#pragma once
#include <memory>
#include <functional>

namespace vshalygin::rpc {
    class transport;

    class iconnector
    {
    public:
        virtual ~iconnector() = default;

        virtual std::unique_ptr<transport> create_transport
                                               (std::function<void()> &&start_callback,
                                                std::function<void()> &&stop_callback) const = 0;

        virtual void interrupt() = 0;
    };
}
