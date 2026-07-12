#pragma once
#include <rpc-lib/internal/service/iservice.h>
#include <gmock/gmock.h>

class service_mock
    : public vshalygin::rpc::internal::iservice
{
public:
    MOCK_METHOD(vshalygin::rpc::future<vshalygin::cl::buffer>,
                process_request_async,
                (vshalygin::cl::buffer &&request_message),
                (override));
};

using service_nice_mock = ::testing::NiceMock<service_mock>;
