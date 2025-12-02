#pragma once
#include <rpc-lib/connection/iconnection.h>
#include <gmock/gmock.h>

class connection_mock
    : public vsh::rpc::iconnection
{
public:
    MOCK_METHOD(void, request_async,
                (vsh::cl::buffer &&, std::function<void(vsh::rpc::request_result, vsh::cl::buffer &&)> &&),
                (override));

    MOCK_METHOD(void, set_request_handler,
                (std::function<vsh::cl::buffer(vsh::cl::buffer &&)> &&handler), (override));

    MOCK_METHOD(void, set_disconnect_handler, (std::function<void()> &&handler), (override));

    MOCK_METHOD(bool, is_connected, (), (const, override));
    MOCK_METHOD(void, disconnect, (), (override));

    MOCK_METHOD(size_t, get_active_requests_count, (), (const, override));
};

using connection_nice_mock = ::testing::NiceMock<connection_mock>;