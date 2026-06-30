#include <common-lib/synchronization/ordered-lock.h>
#include <gtest/gtest.h>

using namespace vshalygin::cl;
using namespace testing;

namespace {
    std::vector<size_t> lock_order;
    std::vector<size_t> unlock_order;

    template<size_t Order>
    class test
    {
    public:
        static constexpr size_t order = Order;

        void lock()
        {
            m_is_locked = true;
            lock_order.push_back(order);
        }

        void unlock()
        {
            m_is_locked = false;
            unlock_order.push_back(order);
        }

        bool is_locked() const
        {
            return m_is_locked;
        }

    private:
        bool m_is_locked = false;
    };
}

class OrderedLock
    : public Test
{
protected:
    void SetUp() override
    {
        lock_order.clear();
        unlock_order.clear();
    }

    void TearDown() override
    {
        ASSERT_EQ(lock_order.size(), unlock_order.size());
        for(size_t i = 0; i < lock_order.size(); ++i) {
            EXPECT_EQ(lock_order[i], unlock_order[lock_order.size() - i - 1]);
        }
    }
};

TEST_F(OrderedLock, BasicTest)
{
    test<0> t1;
    test<1> t2;
    auto lock = ordered_lock(t2, t1);

    EXPECT_TRUE(t1.is_locked());
    EXPECT_TRUE(t2.is_locked());
}

TEST_F(OrderedLock, DefaultConstructible)
{
    ordered_lock<test<0>, test<1>> l;

    EXPECT_FALSE(l.is_locked());
    EXPECT_FALSE(l);
}
