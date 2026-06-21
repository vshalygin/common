#include <common-lib/thread-pool/future.h>
#include <common-lib/thread-pool/thread-pool.h>

#include <gtest/gtest.h>

using namespace vshalygin::cl;
using namespace testing;

TEST(Future, Init)
{
    thread_pool pool(2);

    auto future = pool.post_ex([]() { return 1; });

    ASSERT_EQ(future.get(), 1);
}
