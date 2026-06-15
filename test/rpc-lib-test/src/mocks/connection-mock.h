#pragma once
#include <rpc-lib/connection/iconnection.h>
#include <gmock/gmock.h>

class connection_mock
    : public vshalygin::rpc::iconnection
{
public:
    MOCK_METHOD(void, start_and_set_transport, (std::unique_ptr<vshalygin::rpc::itransport> transport), (override));

    MOCK_METHOD(void, request_async,
                (vshalygin::cl::buffer &&, std::function<void(vshalygin::rpc::request_result, vshalygin::cl::buffer &&)> &&),
                (override));

    MOCK_METHOD(void,
                set_change_state_handler,
                (std::function<void(vshalygin::rpc::connection_state)> &&),
                (override));

    MOCK_METHOD(bool, is_active, (), (const, override));
    MOCK_METHOD(void, stop_transport, (), (override));

    MOCK_METHOD(size_t, get_active_requests_count, (), (const, override));
    MOCK_METHOD(size_t, get_active_timers_count, (), (const, override));
};

using connection_nice_mock = ::testing::NiceMock<connection_mock>;
