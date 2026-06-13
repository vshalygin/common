#pragma once
#include <rpc-lib/listener/ilistener.h>
#include <gmock/gmock.h>

class listener_mock
    : public vshalygin::rpc::ilistener
{
public:
    MOCK_METHOD(void, start, (), (override));
    MOCK_METHOD(void, stop, (), (override));
    MOCK_METHOD(bool, is_stopped, (), (const, override));
    MOCK_METHOD(void, set_change_state_handler, (change_state_handler_t &&handler), (override));

    MOCK_METHOD(void, set_connect_handler, (connect_handler_t &&handler), (override));
};

using listener_nice_mock = ::testing::NiceMock<listener_mock>;
