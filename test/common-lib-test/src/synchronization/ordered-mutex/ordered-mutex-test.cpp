#include <common-lib/synchronization/ordered-mutex.h>
#include <common-lib/synchronization/ordered-lock.h>
#include <gtest/gtest.h>

using namespace vshalygin::cl;

TEST(OrderedMutex, BasicTest)
{
    ordered_mutex<0> mtx1;
    ordered_mutex<1> mtx2;

    auto lock = ordered_lock(mtx2, mtx1);

    lock.unlock();
    lock.lock();
}
