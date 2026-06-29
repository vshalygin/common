//#include <common-lib/synchronization/ordered-lock.h>
//#include <gtest/gtest.h>
//
//using namespace vshalygin::cl;
//using namespace testing;
//
//namespace {
//    class test1
//    {
//    public:
//        static constexpr size_t order = 1;
//
//        void lock()
//        {
//            m_is_locked = true;
//        }
//
//        void unlock()
//        {
//            m_is_locked = false;
//        }
//
//        bool is_locked() const
//        {
//            return m_is_locked;
//        }
//
//    private:
//        bool m_is_locked = false;
//    };
//
//    class test2
//    {
//    public:
//        static constexpr size_t order = 2;
//
//        void lock()
//        {
//            m_is_locked = true;
//        }
//
//        void unlock()
//        {
//            m_is_locked = false;
//        }
//
//        bool is_locked() const
//        {
//            return m_is_locked;
//        }
//
//    private:
//        bool m_is_locked = false;
//    };
//}
//
//TEST(OrderedLock, Init)
//{
//    test1 t1;
//    test2 t2;
//    auto lock = lock_in_order(t2, t1);
//
//    EXPECT_TRUE(t1.is_locked());
//    EXPECT_TRUE(t2.is_locked());
//}
