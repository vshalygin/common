#pragma once
#include <rpc-lib/connection/iconnection.h>

#include <gmock/gmock.h>

class connection_mock
    : public vshalygin::rpc::iconnection
{
public:
    MOCK_METHOD(void, start, (), (override));

    MOCK_METHOD(void, deactivate, (), (override));
    MOCK_METHOD(bool, is_active, (), (const, override));

    MOCK_METHOD(req_result_future, request_async, (vshalygin::cl::buffer &&message), (override));

    MOCK_METHOD(void, set_stop_callback, (vshalygin::cl::thread_pool_task<void()> &&callback), (override));
};

using connection_nice_mock = ::testing::NiceMock<connection_mock>;
