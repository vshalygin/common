#pragma once
#include <rpc-lib/service/iservice.h>
#include <gmock/gmock.h>

class service_mock
    : public vshalygin::rpc::iservice
{
public:
    MOCK_METHOD(void,
                process_request,
                (vshalygin::cl::buffer &&request_message,
                 std::function<void(vshalygin::cl::buffer &&)> &&raw_response_callback),
                (override));
};

using service_nice_mock = ::testing::NiceMock<service_mock>;
