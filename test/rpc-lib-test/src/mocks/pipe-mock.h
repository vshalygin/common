#pragma once
#include "rpc-lib/pipe/ipipe.h"
#include <gmock/gmock.h>

class pipe_mock
    : public vshalygin::rpc::ipipe
{
public:
    MOCK_METHOD(bool, is_connected, (), (const, override));

    MOCK_METHOD(bool, wait_connect_for, (const std::chrono::microseconds &), (const, override));
    MOCK_METHOD(bool, wait_connect, (), (const, override));

    MOCK_METHOD(bool,
                write_async,
                (vshalygin::cl::buffer &&, std::function<void(vshalygin::rpc::pipe_op_res)> &&),
                (override));
    MOCK_METHOD(bool,
                read_async,
                (std::function<void(vshalygin::rpc::pipe_op_res, vshalygin::cl::buffer &&)> &&),
                (override));

    MOCK_METHOD(bool,
                try_to_write_for,
                (vshalygin::cl::buffer &&msg, const std::chrono::microseconds &),
                (override));
    MOCK_METHOD(std::optional<vshalygin::cl::buffer>,
                try_to_read_for,
                (const std::chrono::microseconds &),
                (override));

    MOCK_METHOD(void, invalidate, (), (override));
};

using pipe_nice_mock = testing::NiceMock<pipe_mock>;
