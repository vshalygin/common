#pragma once
#include <rpc-lib/service/iservice.h>
#include <gmock/gmock.h>

class service_mock
    : public vsh::rpc::iservice
{
public:
    MOCK_METHOD(void,
                process_request,
                (vsh::cl::buffer &&request_message,
                 std::function<void(vsh::cl::buffer &&)> &&raw_response_callback),
                (override));
};

using service_nice_mock = ::testing::NiceMock<service_mock>;
