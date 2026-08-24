#pragma once
#include <rpc-lib/internal/service/iservice.h>
#include <gmock/gmock.h>

class service_mock
    : public vshalygin::rpc::internal::iservice
{
public:
    using process_request_async_ret_t = vshalygin::cl::future<vshalygin::cl::thread_pool, vshalygin::cl::buffer>;
    MOCK_METHOD(process_request_async_ret_t,
                process_request_async,
                (vshalygin::cl::buffer &&request_message),
                (override));
};

using service_nice_mock = ::testing::NiceMock<service_mock>;
