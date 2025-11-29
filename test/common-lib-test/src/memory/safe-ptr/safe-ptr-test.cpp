#include <common-lib/memory/safe-ptr/safe-ptr.h>

#include <gtest/gtest.h>

using namespace vsh::cl;
using namespace testing;

namespace {
    class call_type
    {
    public:
        call_type()
        {
            is_instance_exists = true;
        }

        virtual ~call_type()
        {
            is_instance_exists = false;
        }

        call_type(call_type &) = delete;
        call_type &operator=(call_type &) = delete;

        void call()
        {
            m_is_called = true;
        }

        void call_const() const
        {
            m_is_called = true;
        }

        bool is_called() const
        {
            return m_is_called;
        }

        inline static bool is_instance_exists;

    private:
        mutable bool m_is_called = false;
    };

    class derived_call_type
        : public call_type
    {};
}

TEST(SafePtr, GivesAccessToInnerData)
{
    call_type *obj = new call_type;
    safe_ptr<call_type> sut;
    sut.reset(obj);

    sut->call();

    ASSERT_TRUE(obj->is_called());
}

TEST(SafePtr, InitializeInnerDataWithDerivedType)
{
    derived_call_type *obj = new derived_call_type;
    safe_ptr<call_type> sut;
    sut.reset(obj);

    sut->call();

    ASSERT_TRUE(obj->is_called());
}

TEST(SafePtr, DestoysInnerDataOnSelfDestruction)
{
    call_type *obj = new call_type;
    auto psut = std::make_unique<safe_ptr<call_type>>();
    psut->reset(obj);

    psut.reset();

    ASSERT_FALSE(call_type::is_instance_exists);
}

TEST(SafePtr, DestoysInnerDataOnResetOperatiorn)
{
    call_type *obj = new call_type;
    auto psut = std::make_unique<safe_ptr<call_type>>();
    psut->reset(obj);

    psut->reset();

    ASSERT_FALSE(call_type::is_instance_exists);
}

TEST(SafePtr, CallsInnerDataIfObjectIsConst)
{
    call_type *obj = new call_type;
    safe_ptr<call_type> sut;
    sut.reset(obj);
    const safe_ptr<call_type> csut = sut;

    csut->call_const();

    ASSERT_TRUE(obj->is_called());
}

TEST(SafePtr, SwapsValues)
{
    call_type *obj = new call_type;
    safe_ptr<call_type> ptr;
    ptr.reset(obj);
    
    safe_ptr<call_type> sut;
    sut.swap(ptr);
    sut->call();

    ASSERT_TRUE(obj->is_called());
}

TEST(SafePtr, CapturesInnerValueOnCreation)
{
    call_type *obj = new call_type;
    safe_ptr<call_type> sut(obj);

    sut->call();

    ASSERT_TRUE(obj->is_called());
}

TEST(SafePtr, MayBeCreatedWithFreeFunction)
{
    auto sut = make_safe<call_type>();

    sut->call();

    ASSERT_TRUE(sut->is_called());
}
