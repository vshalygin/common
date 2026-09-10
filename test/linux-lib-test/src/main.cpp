#include <gtest/gtest.h>

#include <cstdio>

int main(int argc, char **argv)
{
    std::printf("Running main() from %s\n", __FILE__);
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
