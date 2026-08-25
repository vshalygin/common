#include <gtest/gtest.h>
#include <cstdio>

#ifdef _MSC_VER
#define _CRTDBG_MAP_ALLOC

#include <crtdbg.h>
#include <stdlib.h>
#endif

GTEST_API_ int main(int argc, char **argv) {
#ifdef _MSC_VER
    _CrtSetDbgFlag(
        _CRTDBG_ALLOC_MEM_DF |
        _CRTDBG_LEAK_CHECK_DF);
#endif

    printf("Running main() from %s\n", __FILE__);
    testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}
