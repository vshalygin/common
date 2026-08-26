#include <gtest/gtest.h>
#include <cstdio>

#ifdef _MSC_VER
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

class LeakChecker
    : public testing::EmptyTestEventListener
{
public:
    void OnTestStart(const testing::TestInfo &) override
    {
        _CrtMemCheckpoint(&m_before);
    }

    void OnTestEnd(const testing::TestInfo &info) override
    {
        _CrtMemState after, diff; after; diff;

        _CrtMemCheckpoint(&after);

        if(_CrtMemDifference(&diff, &m_before, &after))
        {
            ADD_FAILURE_AT(
                info.file(),
                info.line())
                << "Memory leak detected";

            _CrtMemDumpStatistics(&diff);
        }
    }

private:
    _CrtMemState m_before;
};
#endif

int main(int argc, char **argv) {
#ifdef _MSC_VER
    testing::UnitTest::GetInstance()
        ->listeners()
        .Append(new LeakChecker);
#endif

    printf("Running main() from %s\n", __FILE__);
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
