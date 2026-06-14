#pragma once
#include "rpc-lib/authenticator/iauthenticator.h"

#include <gmock/gmock.h>

class authenticator_mock
    : public vshalygin::rpc::iauthenticator
{
public:
    MOCK_METHOD(vshalygin::rpc::proto::auth_request, create_request, (), (const, override));
    MOCK_METHOD(bool, check_request, (const vshalygin::rpc::proto::auth_request &req), (const, override));
};

using authenticator_nice_mock = ::testing::NiceMock<authenticator_mock>;
