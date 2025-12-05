#pragma once
#include <rpc-lib/connection/iconnection.h>
#include <gmock/gmock.h>

class connection_mock
    : public vsh::rpc::iconnection
{
public:
    MOCK_METHOD(void, set_and_start_transport, (std::unique_ptr<vsh::rpc::itransport> transport), (override));

    MOCK_METHOD(void, request_async,
                (vsh::cl::buffer &&, std::function<void(vsh::rpc::request_result, vsh::cl::buffer &&)> &&),
                (override));

    MOCK_METHOD(void, set_request_handler,
                (std::function<void(vsh::cl::buffer &&, response_handler_t &&)> &&handler), (override));

    MOCK_METHOD(void,
                set_change_state_handler,
                (std::function<void(vsh::rpc::connection_state)> &&),
                (override));

    MOCK_METHOD(bool, is_active, (), (const, override));
    MOCK_METHOD(void, stop_transport, (), (override));

    MOCK_METHOD(size_t, get_active_requests_count, (), (const, override));
};

using connection_nice_mock = ::testing::NiceMock<connection_mock>;
