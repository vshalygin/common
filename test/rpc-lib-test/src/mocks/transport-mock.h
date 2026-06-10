#pragma once
#include <rpc-lib/interface/itransport.h>
#include <gmock/gmock.h>

class transport_mock
    : public vshalygin::rpc::itransport
{
public:
    MOCK_METHOD(void, send_async, (vshalygin::cl::buffer &&message,
                                   std::function<void()> &&error_handler), (const, override));
    MOCK_METHOD(void, recv_async, (std::function<void(vshalygin::cl::buffer &&)> &&handler), (const, override));

    MOCK_METHOD(void, start, (std::function<void()> &&, std::function<void()> &&), (override));
    MOCK_METHOD(void, stop, (), (override));
    MOCK_METHOD(bool, is_stopped, (), (const, override));
};

using transport_nice_mock = ::testing::NiceMock<transport_mock>;
