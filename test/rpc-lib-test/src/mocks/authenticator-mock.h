#pragma once
#include "rpc-lib/authenticator/iauthenticator.h"

#include <gmock/gmock.h>

class authenticator_mock
    : public vshalygin::rpc::iauthenticator
{
public:
    MOCK_METHOD(vshalygin::cl::buffer, create_request, (), (const, override));
    MOCK_METHOD(vshalygin::cl::buffer, create_response, (vshalygin::cl::cbuffer_view req), (const, override));
    MOCK_METHOD(bool, check_request, (vshalygin::cl::cbuffer_view res), (const, override));
    MOCK_METHOD(bool, check_response, (vshalygin::cl::cbuffer_view res), (const, override));
};

using authenticator_nice_mock = ::testing::NiceMock<authenticator_mock>;
