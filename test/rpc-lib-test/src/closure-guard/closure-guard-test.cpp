#include <rpc-lib/closure-guard/closure-guard.h>

#include "mocks/closure-mock.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace vshalygin::rpc;
using namespace testing;

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

    Mock::VerifyAndClearExpectations(m_closure.get());
}

TEST_F(ClosureGuard, DoesNothingIfClosureIsNullptr)
{
    closure_guard(nullptr);
}

TEST_F(ClosureGuard, CallsRunMethodOnResetOperation)
{
    auto sut = closure_guard(m_closure.get());
    EXPECT_CALL(*m_closure, Run)
        .Times(1);

    sut.reset();

    Mock::VerifyAndClearExpectations(m_closure.get());
}

TEST_F(ClosureGuard, CallsRunMethodOnResetOperationOnlyOnce)
{
    auto sut = closure_guard(m_closure.get());
    EXPECT_CALL(*m_closure, Run)
        .Times(1);

    sut.reset();
    sut.reset();
    sut.reset();

    Mock::VerifyAndClearExpectations(m_closure.get());
}

TEST_F(ClosureGuard, DoesNothingOnResetOperationAfterMoving)
{
    auto sut = closure_guard(m_closure.get());
    closure_guard other(std::move(sut));

    sut.reset();
}

TEST_F(ClosureGuard, CallsDoneIfMoveAssignedByEmtpyGuard)
{
    auto sut = closure_guard(m_closure.get());

    EXPECT_CALL(*m_closure, Run)
        .Times(1);
    sut = closure_guard();

    Mock::VerifyAndClearExpectations(m_closure.get());
}

TEST_F(ClosureGuard, CallsDoneIfResetCalledOnMoveAssignedObject)
{
    auto guard = closure_guard(m_closure.get());
    auto sut = std::move(guard);

    EXPECT_CALL(*m_closure, Run)
        .Times(1);
    sut.reset();

    Mock::VerifyAndClearExpectations(m_closure.get());
}

TEST_F(ClosureGuard, CallsDoneOnlyOnceIfOneObjectWasMoveConstructed)
{
    EXPECT_CALL(*m_closure, Run)
        .Times(1);

    auto guard1 = closure_guard(m_closure.get());
    auto sut = std::move(guard1);
}

TEST_F(ClosureGuard, DoesNothingIfMoveAssignSameObject)
{
    EXPECT_CALL(*m_closure, Run)
        .Times(0);

    auto sut = closure_guard(m_closure.get());
    sut = std::move(sut);

    Mock::VerifyAndClearExpectations(m_closure.get());
}

TEST_F(ClosureGuard, DoesNothingIfWasCreatedByDefaultConstructor)
{
    closure_guard();
}
