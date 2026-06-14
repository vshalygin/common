#pragma once
#include "rpc-lib/pipe/ipipe-env.h"

#include <gmock/gmock.h>

class pipe_env_mock
    : public vshalygin::rpc::ipipe_env
{
public:
    MOCK_METHOD(std::shared_ptr<vshalygin::rpc::ipipe>, create_pipe, (), (override));
    MOCK_METHOD(std::shared_ptr<vshalygin::rpc::ipipe>, open_pipe, (), (override));
};

using pipe_env_nice_mock = ::testing::NiceMock<pipe_env_mock>;
