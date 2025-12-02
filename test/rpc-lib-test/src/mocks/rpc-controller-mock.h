#pragma once
#pragma warning(push, 0)
#include <google/protobuf/service.h>
#pragma warning(pop)

#include <gmock/gmock.h>

class rpc_controller_mock
    : public google::protobuf::RpcController
{
public:
    MOCK_METHOD(void, Reset, (), (override));
    MOCK_METHOD(bool, Failed, (), (const, override));
    MOCK_METHOD(std::string, ErrorText, (), (const, override));
    MOCK_METHOD(void, StartCancel, (), (override));
    MOCK_METHOD(void, SetFailed, (const std::string &reason), (override));
    MOCK_METHOD(bool, IsCanceled, (), (const, override));
    MOCK_METHOD(void, NotifyOnCancel, (google::protobuf::Closure *callback), (override));
};

using rpc_controller_nice_mock = ::testing::NiceMock<rpc_controller_mock>;
