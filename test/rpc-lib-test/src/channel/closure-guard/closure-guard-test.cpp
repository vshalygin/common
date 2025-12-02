#include <rpc-lib/channel/closure-guard/closure-guard.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace vsh::rpc;
using namespace testing;

namespace {
    class closure_mock
        : public google::protobuf::Closure
    {
    public:
        MOCK_METHOD(void, Run, (), (override));
    };

    using closure_nice_mock = NiceMock<closure_mock>;
}

class ClosureGuard
    : public Test
{
protected:
    void SetUp() override
    {
        m_closure = std::make_unique<closure_nice_mock>();
    }

protected:
    std::unique_ptr<closure_nice_mock> m_closure;
};

TEST_F(ClosureGuard, CallsRunMethodOnDestruction)
{
    auto sut = std::make_unique<closure_guard>(m_closure.get());
    EXPECT_CALL(*m_closure, Run)
        .Times(1);

    sut.reset();

    Mock::VerifyAndClearExpectations(&m_closure);
}

TEST_F(ClosureGuard, HasSharedPtrCopingSemantics)
{
    auto sut1 = std::make_unique<closure_guard>(m_closure.get());
    auto sut2 = std::make_unique<closure_guard>(*sut1);
    EXPECT_CALL(*m_closure, Run)
        .Times(0);
    sut1.reset();
    Mock::VerifyAndClearExpectations(&m_closure);

    EXPECT_CALL(*m_closure, Run)
        .Times(1);
    sut2.reset();
    Mock::VerifyAndClearExpectations(&m_closure);
}
