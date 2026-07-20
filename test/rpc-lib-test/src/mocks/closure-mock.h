#pragma once
#pragma warning(push, 0)
#include <google/protobuf/service.h>
#pragma warning(pop)

#include <gmock/gmock.h>

class closure_mock
    : public google::protobuf::Closure
{
public:
    MOCK_METHOD(void, Run, (), (override));
};

using closure_nice_mock = ::testing::NiceMock<closure_mock>;
