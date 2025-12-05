#pragma once
#include <rpc-lib/interface/iconnector.h>
#include <gmock/gmock.h>

class connector_mock
    : public vsh::rpc::iconnector
{
public:
    MOCK_METHOD(std::unique_ptr<vsh::rpc::itransport>, create_transport, (), (override));
};

using connector_nice_mock = ::testing::NiceMock<connector_mock>;
