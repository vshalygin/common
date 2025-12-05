#pragma once
#include <rpc-lib/channel/ichannel.h>
#include <gmock/gmock.h>

class channel_mock
    : public vsh::rpc::ichannel
{
public:
    MOCK_METHOD(void, set_connection, (std::shared_ptr<vsh::rpc::iconnection> connection), (override));
    MOCK_METHOD(std::shared_ptr<vsh::rpc::iconnection>, get_connection, (), (const, override));
    MOCK_METHOD(void, drop_connection, (), (override));

    MOCK_METHOD(void,
                CallMethod,
                (const google::protobuf::MethodDescriptor *method,
                 google::protobuf::RpcController *controller,
                 const google::protobuf::Message *request,
                 google::protobuf::Message *response,
                 google::protobuf::Closure *done),
                (override));
};

using channel_nice_mock = ::testing::NiceMock<channel_mock>;
