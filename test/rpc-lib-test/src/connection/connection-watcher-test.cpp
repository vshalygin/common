#include <rpc-lib/internal/connection/connection-watcher.h>

#include <gtest/gtest.h>

#include <chrono>
#include <thread>

using namespace vshalygin::rpc;

namespace {
    bool wait_until_not_responding(connection_watcher &watcher,
                                   std::chrono::milliseconds timeout)
    {
        const auto deadline = std::chrono::steady_clock::now() + timeout;
        do {
            if(watcher.is_connection_not_responding()) {
                return true;
            }

            std::this_thread::yield();
        } while(std::chrono::steady_clock::now() < deadline);

        return watcher.is_connection_not_responding();
    }
}

TEST(ConnectionWatcher, HasActivityAfterConstruction)
{
    connection_watcher watcher(std::chrono::hours(1));

    EXPECT_TRUE(watcher.check_and_drop_activity_flag());
}

TEST(ConnectionWatcher, DropsActivityFlagAfterCheck)
{
    connection_watcher watcher(std::chrono::hours(1));

    ASSERT_TRUE(watcher.check_and_drop_activity_flag());
    EXPECT_FALSE(watcher.check_and_drop_activity_flag());
    EXPECT_FALSE(watcher.check_and_drop_activity_flag());
}

TEST(ConnectionWatcher, SetActivityFlagRestoresDroppedFlag)
{
    connection_watcher watcher(std::chrono::hours(1));
    ASSERT_TRUE(watcher.check_and_drop_activity_flag());
    ASSERT_FALSE(watcher.check_and_drop_activity_flag());

    watcher.set_activity_flag();

    EXPECT_TRUE(watcher.check_and_drop_activity_flag());
    EXPECT_FALSE(watcher.check_and_drop_activity_flag());
}

TEST(ConnectionWatcher, DoesNotReportTimeoutBeforePingIsWaiting)
{
    connection_watcher watcher(std::chrono::milliseconds(0));

    EXPECT_FALSE(watcher.is_connection_not_responding());
}

TEST(ConnectionWatcher, DoesNotReportTimeoutBeforePingPeriodExpires)
{
    connection_watcher watcher(std::chrono::hours(1));

    watcher.set_ping_waiting();

    EXPECT_FALSE(watcher.is_connection_not_responding());
}

TEST(ConnectionWatcher, ReportsTimeoutAfterPingPeriodExpires)
{
    connection_watcher watcher(std::chrono::milliseconds(1));
    watcher.set_ping_waiting();

    EXPECT_TRUE(wait_until_not_responding(watcher, std::chrono::seconds(1)));
}

TEST(ConnectionWatcher, RepeatedSetPingWaitingDoesNotRestartTimeout)
{
    connection_watcher watcher(std::chrono::milliseconds(20));
    watcher.set_ping_waiting();
    ASSERT_TRUE(wait_until_not_responding(watcher, std::chrono::seconds(1)));

    watcher.set_ping_waiting();

    EXPECT_TRUE(watcher.is_connection_not_responding());
}

TEST(ConnectionWatcher, ActivityClearsPingWaitingAndTimeout)
{
    connection_watcher watcher(std::chrono::milliseconds(1));
    watcher.set_ping_waiting();
    ASSERT_TRUE(wait_until_not_responding(watcher, std::chrono::seconds(1)));

    watcher.set_activity_flag();

    EXPECT_FALSE(watcher.is_connection_not_responding());
    EXPECT_TRUE(watcher.check_and_drop_activity_flag());
}
