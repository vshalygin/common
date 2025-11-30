#pragma once
#include <common-lib/timer/multiple-timer/imultiple-timer.h>
#include <gmock/gmock.h>

class multiple_timer_mock
    : public vsh::cl::imultiple_timer
{
public:
    MOCK_METHOD(uint64_t, start, (callback_t &&callback,
                                  const std::chrono::microseconds &microseconds), (override));
    MOCK_METHOD(void, cancel, (uint64_t id), (override));
    MOCK_METHOD(void, cancel_all, (), (override));

    MOCK_METHOD(size_t, get_active_timers_count, (), (const, override));
};

using multiple_timer_nice_mock = ::testing::NiceMock<multiple_timer_mock>;
