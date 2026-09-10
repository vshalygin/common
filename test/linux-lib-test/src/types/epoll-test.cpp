#include <linux-lib/types/epoll.h>

#include <gtest/gtest.h>

#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <fcntl.h>
#include <unistd.h>

#include <array>
#include <cerrno>
#include <cstdint>
#include <stdexcept>
#include <system_error>
#include <type_traits>
#include <utility>

using namespace testing;
using namespace vshalygin::linux;

static_assert(!std::is_copy_constructible_v<epoll>);
static_assert(!std::is_copy_assignable_v<epoll>);
static_assert(std::is_nothrow_move_constructible_v<epoll>);
static_assert(std::is_nothrow_move_assignable_v<epoll>);
static_assert(noexcept(std::declval<epoll &>().add(
    std::declval<unique_fd::fd_type>(),
    std::declval<uint32_t>(),
    std::declval<void *>(),
    std::declval<std::error_code &>())));
static_assert(noexcept(std::declval<epoll &>().modify(
    std::declval<unique_fd::fd_type>(),
    std::declval<uint32_t>(),
    std::declval<void *>(),
    std::declval<std::error_code &>())));
static_assert(noexcept(std::declval<epoll &>().remove(
    std::declval<unique_fd::fd_type>(),
    std::declval<std::error_code &>())));
static_assert(noexcept(std::declval<epoll &>().wait(
    std::declval<epoll_event *>(),
    std::declval<int>(),
    std::declval<std::error_code &>(),
    std::declval<int>())));

namespace {
    constexpr unique_fd::fd_type invalid_fd = -1;
    constexpr int wait_timeout_ms = 100;

    unique_fd create_event_descriptor(unsigned int initial_value = 0)
    {
        const auto fd = ::eventfd(
            initial_value,
            EFD_NONBLOCK | EFD_CLOEXEC);
        if(fd == invalid_fd) {
            throw std::system_error(
                errno,
                std::generic_category(),
                "eventfd");
        }

        return unique_fd(fd);
    }

    bool is_descriptor_open(unique_fd::fd_type fd)
    {
        errno = 0;
        const auto result = ::fcntl(fd, F_GETFD);
        if(result != -1) {
            return true;
        }
        if(errno == EBADF) {
            return false;
        }

        throw std::system_error(
            errno,
            std::generic_category(),
            "fcntl(F_GETFD)");
    }

    void notify(unique_fd::fd_type fd)
    {
        constexpr uint64_t value = 1;
        ASSERT_EQ(::write(fd, &value, sizeof(value)),
                  static_cast<ssize_t>(sizeof(value)));
    }

    void consume(unique_fd::fd_type fd)
    {
        uint64_t value = 0;
        ASSERT_EQ(::read(fd, &value, sizeof(value)),
                  static_cast<ssize_t>(sizeof(value)));
        EXPECT_GT(value, 0u);
    }

    epoll_event wait_for_single_event(epoll &instance)
    {
        epoll_event event{};
        const auto count = instance.wait(
            &event,
            1,
            wait_timeout_ms);
        if(count != 1) {
            throw std::runtime_error("Expected exactly one epoll event");
        }

        return event;
    }
}

TEST(Epoll, ConstructorCreatesOpenDescriptorWithCloseOnExecFlag)
{
    epoll instance;

    ASSERT_NE(instance.get(), invalid_fd);
    ASSERT_TRUE(is_descriptor_open(instance.get()));

    const auto flags = ::fcntl(instance.get(), F_GETFD);
    ASSERT_NE(flags, -1);
    EXPECT_NE(flags & FD_CLOEXEC, 0);
}

TEST(Epoll, DestructorClosesOwnedDescriptor)
{
    unique_fd::fd_type raw_fd = invalid_fd;

    {
        epoll instance;
        raw_fd = instance.get();
        ASSERT_TRUE(is_descriptor_open(raw_fd));
    }

    EXPECT_FALSE(is_descriptor_open(raw_fd));
}

TEST(Epoll, MoveConstructorTransfersDescriptorOwnership)
{
    epoll source;
    const auto raw_fd = source.get();

    epoll destination(std::move(source));

    EXPECT_EQ(source.get(), invalid_fd);
    EXPECT_EQ(destination.get(), raw_fd);
    EXPECT_TRUE(is_descriptor_open(raw_fd));
}

TEST(Epoll, MoveAssignmentClosesDestinationAndTransfersOwnership)
{
    epoll source;
    epoll destination;
    const auto source_fd = source.get();
    const auto destination_fd = destination.get();

    destination = std::move(source);

    EXPECT_EQ(source.get(), invalid_fd);
    EXPECT_EQ(destination.get(), source_fd);
    EXPECT_TRUE(is_descriptor_open(source_fd));
    EXPECT_FALSE(is_descriptor_open(destination_fd));
}

TEST(Epoll, SelfMoveAssignmentPreservesDescriptor)
{
    epoll instance;
    const auto raw_fd = instance.get();

    auto &same_instance = instance;
    instance = std::move(same_instance);

    EXPECT_EQ(instance.get(), raw_fd);
    EXPECT_TRUE(is_descriptor_open(raw_fd));
}

TEST(Epoll, AddReportsReadableDescriptorAndPreservesDataPointer)
{
    epoll instance;
    auto event_fd = create_event_descriptor(1);
    int user_data = 42;

    instance.add(event_fd.get(), EPOLLIN, &user_data);
    const auto event = wait_for_single_event(instance);

    EXPECT_NE(event.events & EPOLLIN, 0u);
    EXPECT_EQ(event.data.ptr, &user_data);
}

TEST(Epoll, AddNoexceptOverloadClearsOldErrorOnSuccess)
{
    epoll instance;
    auto event_fd = create_event_descriptor();
    std::error_code ec = std::make_error_code(std::errc::io_error);

    instance.add(event_fd.get(), EPOLLIN, nullptr, ec);

    EXPECT_FALSE(ec);
}

TEST(Epoll, AddNoexceptOverloadReportsDuplicateRegistration)
{
    epoll instance;
    auto event_fd = create_event_descriptor();
    instance.add(event_fd.get(), EPOLLIN, nullptr);
    std::error_code ec;

    instance.add(event_fd.get(), EPOLLIN, nullptr, ec);

    ASSERT_TRUE(ec);
    EXPECT_EQ(ec.value(), EEXIST);
    EXPECT_EQ(ec.category(), std::generic_category());
}

TEST(Epoll, AddNoexceptOverloadReportsInvalidDescriptor)
{
    epoll instance;
    std::error_code ec;

    instance.add(invalid_fd, EPOLLIN, nullptr, ec);

    ASSERT_TRUE(ec);
    EXPECT_EQ(ec.value(), EBADF);
    EXPECT_EQ(ec.category(), std::generic_category());
}

TEST(Epoll, ThrowingAddReportsDuplicateRegistration)
{
    epoll instance;
    auto event_fd = create_event_descriptor();
    instance.add(event_fd.get(), EPOLLIN, nullptr);

    try {
        instance.add(event_fd.get(), EPOLLIN, nullptr);
        FAIL() << "Expected std::system_error";
    } catch(const std::system_error &error) {
        EXPECT_EQ(error.code().value(), EEXIST);
        EXPECT_EQ(error.code().category(), std::generic_category());
    }
}

TEST(Epoll, ModifyReplacesStoredDataPointer)
{
    epoll instance;
    auto event_fd = create_event_descriptor(1);
    int old_data = 1;
    int new_data = 2;
    instance.add(event_fd.get(), EPOLLIN, &old_data);

    instance.modify(event_fd.get(), EPOLLIN, &new_data);
    const auto event = wait_for_single_event(instance);

    EXPECT_NE(event.events & EPOLLIN, 0u);
    EXPECT_EQ(event.data.ptr, &new_data);
}

TEST(Epoll, ModifyReplacesEventMask)
{
    epoll instance;
    auto event_fd = create_event_descriptor();
    int user_data = 3;
    instance.add(event_fd.get(), EPOLLIN, nullptr);

    epoll_event event{};
    ASSERT_EQ(instance.wait(&event, 1, 0), 0);

    instance.modify(event_fd.get(), EPOLLOUT, &user_data);
    const auto modified_event = wait_for_single_event(instance);

    EXPECT_NE(modified_event.events & EPOLLOUT, 0u);
    EXPECT_EQ(modified_event.events & EPOLLIN, 0u);
    EXPECT_EQ(modified_event.data.ptr, &user_data);
}

TEST(Epoll, ModifyNoexceptOverloadClearsOldErrorOnSuccess)
{
    epoll instance;
    auto event_fd = create_event_descriptor();
    instance.add(event_fd.get(), EPOLLIN, nullptr);
    std::error_code ec = std::make_error_code(std::errc::io_error);

    instance.modify(event_fd.get(), EPOLLIN, nullptr, ec);

    EXPECT_FALSE(ec);
}

TEST(Epoll, ModifyNoexceptOverloadReportsUnregisteredDescriptor)
{
    epoll instance;
    auto event_fd = create_event_descriptor();
    std::error_code ec;

    instance.modify(event_fd.get(), EPOLLIN, nullptr, ec);

    ASSERT_TRUE(ec);
    EXPECT_EQ(ec.value(), ENOENT);
    EXPECT_EQ(ec.category(), std::generic_category());
}

TEST(Epoll, ThrowingModifyReportsUnregisteredDescriptor)
{
    epoll instance;
    auto event_fd = create_event_descriptor();

    EXPECT_THROW(
        instance.modify(event_fd.get(), EPOLLIN, nullptr),
        std::system_error);
}

TEST(Epoll, RemoveStopsDeliveringEvents)
{
    epoll instance;
    auto event_fd = create_event_descriptor(1);
    instance.add(event_fd.get(), EPOLLIN, nullptr);

    instance.remove(event_fd.get());

    epoll_event event{};
    EXPECT_EQ(instance.wait(&event, 1, 0), 0);
}

TEST(Epoll, RemoveDoesNotCloseRemovedDescriptor)
{
    epoll instance;
    auto event_fd = create_event_descriptor();
    instance.add(event_fd.get(), EPOLLIN, nullptr);

    instance.remove(event_fd.get());

    EXPECT_TRUE(is_descriptor_open(event_fd.get()));
    notify(event_fd.get());
    consume(event_fd.get());
}

TEST(Epoll, DestructorDoesNotCloseRegisteredDescriptor)
{
    auto event_fd = create_event_descriptor();

    {
        epoll instance;
        instance.add(event_fd.get(), EPOLLIN, nullptr);
    }

    EXPECT_TRUE(is_descriptor_open(event_fd.get()));
    notify(event_fd.get());
    consume(event_fd.get());
}

TEST(Epoll, RemoveNoexceptOverloadClearsOldErrorOnSuccess)
{
    epoll instance;
    auto event_fd = create_event_descriptor();
    instance.add(event_fd.get(), EPOLLIN, nullptr);
    std::error_code ec = std::make_error_code(std::errc::io_error);

    instance.remove(event_fd.get(), ec);

    EXPECT_FALSE(ec);
}

TEST(Epoll, RemoveNoexceptOverloadReportsUnregisteredDescriptor)
{
    epoll instance;
    auto event_fd = create_event_descriptor();
    std::error_code ec;

    instance.remove(event_fd.get(), ec);

    ASSERT_TRUE(ec);
    EXPECT_EQ(ec.value(), ENOENT);
    EXPECT_EQ(ec.category(), std::generic_category());
}

TEST(Epoll, ThrowingRemoveReportsUnregisteredDescriptor)
{
    epoll instance;
    auto event_fd = create_event_descriptor();

    EXPECT_THROW(
        instance.remove(event_fd.get()),
        std::system_error);
}

TEST(Epoll, LevelTriggeredRegistrationRemainsReadyUntilDataIsConsumed)
{
    epoll instance;
    auto event_fd = create_event_descriptor(1);
    instance.add(event_fd.get(), EPOLLIN, nullptr);

    EXPECT_NE(wait_for_single_event(instance).events & EPOLLIN, 0u);
    EXPECT_NE(wait_for_single_event(instance).events & EPOLLIN, 0u);

    consume(event_fd.get());

    epoll_event event{};
    EXPECT_EQ(instance.wait(&event, 1, 0), 0);
}

TEST(Epoll, OneShotRegistrationRequiresRearming)
{
    epoll instance;
    auto event_fd = create_event_descriptor(1);
    int first_data = 1;
    int second_data = 2;
    instance.add(
        event_fd.get(),
        EPOLLIN | EPOLLONESHOT,
        &first_data);

    const auto first_event = wait_for_single_event(instance);
    EXPECT_EQ(first_event.data.ptr, &first_data);

    epoll_event event{};
    EXPECT_EQ(instance.wait(&event, 1, 0), 0);

    instance.modify(
        event_fd.get(),
        EPOLLIN | EPOLLONESHOT,
        &second_data);
    const auto second_event = wait_for_single_event(instance);
    EXPECT_EQ(second_event.data.ptr, &second_data);
}

TEST(Epoll, WaitReturnsAllReadyDescriptorsWithinCapacity)
{
    epoll instance;
    auto first_fd = create_event_descriptor(1);
    auto second_fd = create_event_descriptor(1);
    int first_data = 1;
    int second_data = 2;
    instance.add(first_fd.get(), EPOLLIN, &first_data);
    instance.add(second_fd.get(), EPOLLIN, &second_data);
    std::array<epoll_event, 2> events{};

    const auto count = instance.wait(
        events.data(),
        static_cast<int>(events.size()),
        wait_timeout_ms);

    ASSERT_EQ(count, 2);
    const auto first_seen =
        events[0].data.ptr == &first_data ||
        events[1].data.ptr == &first_data;
    const auto second_seen =
        events[0].data.ptr == &second_data ||
        events[1].data.ptr == &second_data;
    EXPECT_TRUE(first_seen);
    EXPECT_TRUE(second_seen);
}

TEST(Epoll, WaitNeverReturnsMoreEventsThanCapacity)
{
    epoll instance;
    auto first_fd = create_event_descriptor(1);
    auto second_fd = create_event_descriptor(1);
    instance.add(first_fd.get(), EPOLLIN, nullptr);
    instance.add(second_fd.get(), EPOLLIN, nullptr);
    epoll_event event{};

    EXPECT_EQ(instance.wait(&event, 1, wait_timeout_ms), 1);
}

TEST(Epoll, WaitReturnsZeroWhenTimeoutExpiresWithoutEvents)
{
    epoll instance;
    epoll_event event{};

    EXPECT_EQ(instance.wait(&event, 1, 0), 0);
}

TEST(Epoll, WaitNoexceptOverloadClearsOldErrorOnTimeout)
{
    epoll instance;
    epoll_event event{};
    std::error_code ec = std::make_error_code(std::errc::io_error);

    const auto count = instance.wait(&event, 1, ec, 0);

    EXPECT_EQ(count, 0);
    EXPECT_FALSE(ec);
}

TEST(Epoll, WaitNoexceptOverloadReportsInvalidCapacity)
{
    epoll instance;
    epoll_event event{};
    std::error_code ec;

    const auto count = instance.wait(&event, 0, ec, 0);

    EXPECT_EQ(count, -1);
    ASSERT_TRUE(ec);
    EXPECT_EQ(ec.value(), EINVAL);
    EXPECT_EQ(ec.category(), std::generic_category());
}

TEST(Epoll, WaitNoexceptOverloadReportsMovedFromInstance)
{
    epoll source;
    epoll destination(std::move(source));
    epoll_event event{};
    std::error_code ec;

    const auto count = source.wait(&event, 1, ec, 0);

    EXPECT_EQ(count, -1);
    ASSERT_TRUE(ec);
    EXPECT_EQ(ec.value(), EBADF);
    EXPECT_EQ(ec.category(), std::generic_category());
    EXPECT_NE(destination.get(), invalid_fd);
}

TEST(Epoll, ThrowingWaitReportsInvalidCapacity)
{
    epoll instance;
    epoll_event event{};

    EXPECT_THROW(instance.wait(&event, 0, 0), std::system_error);
}

TEST(Epoll, WaitUsesDefaultInfiniteTimeoutForAlreadyReadyDescriptor)
{
    epoll instance;
    auto event_fd = create_event_descriptor(1);
    instance.add(event_fd.get(), EPOLLIN, nullptr);
    epoll_event event{};

    const auto count = instance.wait(&event, 1);

    ASSERT_EQ(count, 1);
    EXPECT_NE(event.events & EPOLLIN, 0u);
}

TEST(Epoll, WaitTreatsAnyNegativeTimeoutAsInfinite)
{
    epoll instance;
    auto event_fd = create_event_descriptor(1);
    instance.add(event_fd.get(), EPOLLIN, nullptr);
    epoll_event event{};

    const auto count = instance.wait(&event, 1, -2);

    ASSERT_EQ(count, 1);
    EXPECT_NE(event.events & EPOLLIN, 0u);
}

TEST(Epoll, DescriptorCanBeNotifiedAfterRegistration)
{
    epoll instance;
    auto event_fd = create_event_descriptor();
    instance.add(event_fd.get(), EPOLLIN, nullptr);

    epoll_event event{};
    ASSERT_EQ(instance.wait(&event, 1, 0), 0);

    notify(event_fd.get());

    ASSERT_EQ(instance.wait(&event, 1, wait_timeout_ms), 1);
    EXPECT_NE(event.events & EPOLLIN, 0u);
}
