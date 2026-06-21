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

TEST(Future, ExecutesSuccessCallback)
{
    thread_pool pool(2);
    int i = 0;
    pool.post_ex([]() { return 2; })
        .then<int>([&i](int &&ii) { i = ii; return 0; })
        .get();

    ASSERT_EQ(i, 2);
}