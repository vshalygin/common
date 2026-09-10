#include <linux-lib/types/unique-fd.h>

#include <gtest/gtest.h>

#include <sys/eventfd.h>
#include <fcntl.h>
#include <unistd.h>

#include <cerrno>
#include <cstdint>
#include <system_error>
#include <type_traits>
#include <utility>

using namespace testing;
using namespace vshalygin::linux;

static_assert(std::is_nothrow_default_constructible_v<unique_fd>);
static_assert(std::is_nothrow_constructible_v<unique_fd, unique_fd::fd_type>);
static_assert(std::is_nothrow_destructible_v<unique_fd>);
static_assert(!std::is_copy_constructible_v<unique_fd>);
static_assert(!std::is_copy_assignable_v<unique_fd>);
static_assert(std::is_nothrow_move_constructible_v<unique_fd>);
static_assert(std::is_nothrow_move_assignable_v<unique_fd>);
static_assert(std::is_nothrow_swappable_v<unique_fd>);

namespace {
    constexpr unique_fd::fd_type invalid_fd = -1;

    unique_fd::fd_type create_descriptor()
    {
        const auto fd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
        if(fd == invalid_fd) {
            throw std::system_error(
                errno,
                std::generic_category(),
                "eventfd");
        }

        return fd;
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

    void write_and_read_event(unique_fd::fd_type fd)
    {
        constexpr uint64_t sent = 7;
        ASSERT_EQ(::write(fd, &sent, sizeof(sent)),
                  static_cast<ssize_t>(sizeof(sent)));

        uint64_t received = 0;
        ASSERT_EQ(::read(fd, &received, sizeof(received)),
                  static_cast<ssize_t>(sizeof(received)));
        EXPECT_EQ(received, sent);
    }
}

TEST(UniqueFd, DefaultConstructedObjectIsEmpty)
{
    unique_fd fd;

    EXPECT_FALSE(fd);
    EXPECT_TRUE(fd.empty());
    EXPECT_EQ(fd.get(), invalid_fd);
}

TEST(UniqueFd, ExplicitlyConstructedObjectOwnsDescriptor)
{
    const auto raw_fd = create_descriptor();
    unique_fd fd(raw_fd);

    EXPECT_TRUE(fd);
    EXPECT_FALSE(fd.empty());
    EXPECT_EQ(fd.get(), raw_fd);
    EXPECT_TRUE(is_descriptor_open(raw_fd));
    write_and_read_event(fd.get());
}

TEST(UniqueFd, DestructorClosesOwnedDescriptor)
{
    const auto raw_fd = create_descriptor();

    {
        unique_fd fd(raw_fd);
        ASSERT_TRUE(is_descriptor_open(raw_fd));
    }

    EXPECT_FALSE(is_descriptor_open(raw_fd));
}

TEST(UniqueFd, MoveConstructorTransfersDescriptorOwnership)
{
    const auto raw_fd = create_descriptor();
    unique_fd source(raw_fd);
    unique_fd destination(std::move(source));

    EXPECT_FALSE(source);
    EXPECT_TRUE(source.empty());
    EXPECT_EQ(source.get(), invalid_fd);
    EXPECT_TRUE(destination);
    EXPECT_EQ(destination.get(), raw_fd);
    EXPECT_TRUE(is_descriptor_open(raw_fd));
    write_and_read_event(destination.get());
}

TEST(UniqueFd, MoveConstructorPreservesEmptyState)
{
    unique_fd source;
    unique_fd destination(std::move(source));

    EXPECT_FALSE(source);
    EXPECT_TRUE(source.empty());
    EXPECT_FALSE(destination);
    EXPECT_TRUE(destination.empty());
}

TEST(UniqueFd, MoveAssignmentTransfersDescriptorOwnership)
{
    const auto raw_fd = create_descriptor();
    unique_fd source(raw_fd);
    unique_fd destination;

    destination = std::move(source);

    EXPECT_FALSE(source);
    EXPECT_EQ(source.get(), invalid_fd);
    EXPECT_TRUE(destination);
    EXPECT_EQ(destination.get(), raw_fd);
    EXPECT_TRUE(is_descriptor_open(raw_fd));
}

TEST(UniqueFd, MoveAssignmentClosesPreviouslyOwnedDescriptor)
{
    const auto source_fd = create_descriptor();
    const auto destination_fd = create_descriptor();
    unique_fd source(source_fd);
    unique_fd destination(destination_fd);

    destination = std::move(source);

    EXPECT_FALSE(source);
    EXPECT_EQ(destination.get(), source_fd);
    EXPECT_TRUE(is_descriptor_open(source_fd));
    EXPECT_FALSE(is_descriptor_open(destination_fd));
}

TEST(UniqueFd, MoveAssignmentFromEmptyObjectClosesDestination)
{
    const auto destination_fd = create_descriptor();
    unique_fd source;
    unique_fd destination(destination_fd);

    destination = std::move(source);

    EXPECT_FALSE(source);
    EXPECT_FALSE(destination);
    EXPECT_FALSE(is_descriptor_open(destination_fd));
}

TEST(UniqueFd, SelfMoveAssignmentPreservesDescriptor)
{
    const auto raw_fd = create_descriptor();
    unique_fd fd(raw_fd);

    auto &same_object = fd;
    fd = std::move(same_object);

    EXPECT_TRUE(fd);
    EXPECT_EQ(fd.get(), raw_fd);
    EXPECT_TRUE(is_descriptor_open(raw_fd));
}

TEST(UniqueFd, ResetWithoutArgumentClosesDescriptorAndMakesObjectEmpty)
{
    const auto raw_fd = create_descriptor();
    unique_fd fd(raw_fd);

    fd.reset();

    EXPECT_FALSE(fd);
    EXPECT_TRUE(fd.empty());
    EXPECT_EQ(fd.get(), invalid_fd);
    EXPECT_FALSE(is_descriptor_open(raw_fd));
}

TEST(UniqueFd, RepeatedResetIsSafeForEmptyObject)
{
    unique_fd fd;

    fd.reset();
    fd.reset();

    EXPECT_FALSE(fd);
    EXPECT_TRUE(fd.empty());
    EXPECT_EQ(fd.get(), invalid_fd);
}

TEST(UniqueFd, ResetAdoptsDescriptorWhenObjectIsEmpty)
{
    const auto raw_fd = create_descriptor();
    unique_fd fd;

    fd.reset(raw_fd);

    EXPECT_TRUE(fd);
    EXPECT_EQ(fd.get(), raw_fd);
    EXPECT_TRUE(is_descriptor_open(raw_fd));
}

TEST(UniqueFd, ResetReplacesAndClosesPreviouslyOwnedDescriptor)
{
    const auto old_fd = create_descriptor();
    const auto new_fd = create_descriptor();
    unique_fd fd(old_fd);

    fd.reset(new_fd);

    EXPECT_EQ(fd.get(), new_fd);
    EXPECT_FALSE(is_descriptor_open(old_fd));
    EXPECT_TRUE(is_descriptor_open(new_fd));
}

TEST(UniqueFd, ResetWithSameDescriptorDoesNotCloseIt)
{
    const auto raw_fd = create_descriptor();
    unique_fd fd(raw_fd);

    fd.reset(raw_fd);

    EXPECT_TRUE(fd);
    EXPECT_EQ(fd.get(), raw_fd);
    EXPECT_TRUE(is_descriptor_open(raw_fd));
    write_and_read_event(fd.get());
}

TEST(UniqueFd, GetDoesNotTransferOwnership)
{
    const auto raw_fd = create_descriptor();

    {
        const unique_fd fd(raw_fd);
        EXPECT_EQ(fd.get(), raw_fd);
        EXPECT_TRUE(is_descriptor_open(fd.get()));
    }

    EXPECT_FALSE(is_descriptor_open(raw_fd));
}

TEST(UniqueFd, ReleaseReturnsDescriptorAndMakesObjectEmpty)
{
    const auto raw_fd = create_descriptor();
    unique_fd fd(raw_fd);

    const auto released_fd = fd.release();

    EXPECT_EQ(released_fd, raw_fd);
    EXPECT_FALSE(fd);
    EXPECT_TRUE(fd.empty());
    EXPECT_EQ(fd.get(), invalid_fd);
    ASSERT_TRUE(is_descriptor_open(released_fd));
    EXPECT_EQ(::close(released_fd), 0);
}

TEST(UniqueFd, DestructorDoesNotCloseReleasedDescriptor)
{
    const auto raw_fd = create_descriptor();
    unique_fd::fd_type released_fd = invalid_fd;

    {
        unique_fd fd(raw_fd);
        released_fd = fd.release();
    }

    ASSERT_EQ(released_fd, raw_fd);
    ASSERT_TRUE(is_descriptor_open(released_fd));
    EXPECT_EQ(::close(released_fd), 0);
}

TEST(UniqueFd, ReleaseFromEmptyObjectReturnsInvalidDescriptor)
{
    unique_fd fd;

    EXPECT_EQ(fd.release(), invalid_fd);
    EXPECT_FALSE(fd);
    EXPECT_TRUE(fd.empty());
}

TEST(UniqueFd, PutOnEmptyObjectReturnsAddressContainingInvalidDescriptor)
{
    unique_fd fd;

    auto *const address = fd.put();

    ASSERT_NE(address, nullptr);
    EXPECT_EQ(address, fd.addressof());
    EXPECT_EQ(*address, invalid_fd);
    EXPECT_FALSE(fd);
}

TEST(UniqueFd, PutClosesPreviouslyOwnedDescriptor)
{
    const auto raw_fd = create_descriptor();
    unique_fd fd(raw_fd);

    auto *const address = fd.put();

    ASSERT_NE(address, nullptr);
    EXPECT_EQ(*address, invalid_fd);
    EXPECT_FALSE(fd);
    EXPECT_FALSE(is_descriptor_open(raw_fd));
}

TEST(UniqueFd, PutAllowsAdoptingDescriptorWrittenThroughReturnedAddress)
{
    const auto raw_fd = create_descriptor();
    unique_fd fd;

    auto *const address = fd.put();
    *address = raw_fd;

    EXPECT_TRUE(fd);
    EXPECT_EQ(fd.get(), raw_fd);
    EXPECT_TRUE(is_descriptor_open(raw_fd));
    write_and_read_event(fd.get());
}

TEST(UniqueFd, PutClosesOldDescriptorBeforeAdoptingNewDescriptor)
{
    const auto old_fd = create_descriptor();
    const auto new_fd = create_descriptor();
    unique_fd fd(old_fd);

    auto *const address = fd.put();
    ASSERT_FALSE(is_descriptor_open(old_fd));
    *address = new_fd;

    EXPECT_EQ(fd.get(), new_fd);
    EXPECT_TRUE(is_descriptor_open(new_fd));
}

TEST(UniqueFd, AddressOfReturnsStoredDescriptorAddressWithoutClosingIt)
{
    const auto raw_fd = create_descriptor();
    unique_fd fd(raw_fd);

    auto *const address = fd.addressof();

    ASSERT_NE(address, nullptr);
    EXPECT_EQ(*address, raw_fd);
    EXPECT_EQ(fd.get(), raw_fd);
    EXPECT_TRUE(is_descriptor_open(raw_fd));
}

TEST(UniqueFd, AddressOfEmptyObjectContainsInvalidDescriptor)
{
    unique_fd fd;

    auto *const address = fd.addressof();

    ASSERT_NE(address, nullptr);
    EXPECT_EQ(*address, invalid_fd);
    EXPECT_FALSE(fd);
}

TEST(UniqueFd, MemberSwapExchangesOwnedDescriptors)
{
    const auto first_fd = create_descriptor();
    const auto second_fd = create_descriptor();
    unique_fd first(first_fd);
    unique_fd second(second_fd);

    first.swap(second);

    EXPECT_EQ(first.get(), second_fd);
    EXPECT_EQ(second.get(), first_fd);
    EXPECT_TRUE(is_descriptor_open(first_fd));
    EXPECT_TRUE(is_descriptor_open(second_fd));
}

TEST(UniqueFd, MemberSwapExchangesOwnedAndEmptyStates)
{
    const auto raw_fd = create_descriptor();
    unique_fd owner(raw_fd);
    unique_fd empty;

    owner.swap(empty);

    EXPECT_FALSE(owner);
    EXPECT_TRUE(empty);
    EXPECT_EQ(empty.get(), raw_fd);
    EXPECT_TRUE(is_descriptor_open(raw_fd));
}

TEST(UniqueFd, FreeSwapUsesArgumentDependentLookup)
{
    const auto first_fd = create_descriptor();
    const auto second_fd = create_descriptor();
    unique_fd first(first_fd);
    unique_fd second(second_fd);

    using std::swap;
    swap(first, second);

    EXPECT_EQ(first.get(), second_fd);
    EXPECT_EQ(second.get(), first_fd);
    EXPECT_TRUE(is_descriptor_open(first_fd));
    EXPECT_TRUE(is_descriptor_open(second_fd));
}

TEST(UniqueFd, BooleanStateTracksOwnershipTransitions)
{
    const auto raw_fd = create_descriptor();
    unique_fd fd;

    EXPECT_FALSE(fd);

    fd.reset(raw_fd);
    EXPECT_TRUE(fd);

    const auto released_fd = fd.release();
    EXPECT_FALSE(fd);

    fd.reset(released_fd);
    EXPECT_TRUE(fd);

    fd.reset();
    EXPECT_FALSE(fd);
    EXPECT_FALSE(is_descriptor_open(raw_fd));
}
