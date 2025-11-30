#pragma once
#include <rpc-lib/connection/itransport.h>
#include <gmock/gmock.h>

class transport_mock
    : public vsh::rpc::itransport
{
public:
    MOCK_METHOD(void, send_async, (vsh::cl::buffer &&message,
                                   std::function<void()> &&error_handler), (const, override));
    MOCK_METHOD(void, recv_async, (std::function<void(vsh::cl::buffer &&)> &&handler), (const, override));

    MOCK_METHOD(void, stop, (), (override));
    MOCK_METHOD(bool, is_stopped, (), (const, override));

    MOCK_METHOD(void, set_stop_handler, (std::function<void()> &&handler), (override));
};

using transport_nice_mock = ::testing::NiceMock<transport_mock>;