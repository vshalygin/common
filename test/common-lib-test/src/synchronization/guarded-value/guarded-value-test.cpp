#include <common-lib/synchronization/guarded-value/guarded-value.h>

#include <gtest/gtest.h>

using namespace vshalygin::cl;
using namespace testing;

namespace {
    class test_class
    {
    public:
        explicit test_class(int numb)
            : number(numb)
        {}

        test_class(test_class &) = delete;
        test_class &operator=(test_class &) = delete;

        test_class(test_class &&) = default;
        test_class &operator=(test_class &&) = default;

        int number;
    };
}

TEST(GuardedValue, MayBeCreatedByMovingValue)
{
    test_class obj(34);

    guarded_value<test_class> sut(std::move(obj));
    auto [guard, val] = sut.get();

    ASSERT_EQ(val.number, 34);
}

TEST(GuardedValue, MayBeCreatedByValueCreationArguments)
{
    guarded_value<test_class> sut(34);
    auto [guard, val] = sut.get();

    ASSERT_EQ(val.number, 34);
}
