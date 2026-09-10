set(linux_lib_test_sources
    src/main.cpp
    src/types/epoll-test.cpp
    src/types/unique-fd-test.cpp
)

add_executable(linux-lib-test
    ${linux_lib_test_sources}
)

target_include_directories(linux-lib-test PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)
target_link_libraries(linux-lib-test PRIVATE
    linux-lib
    GTest::gtest
    GTest::gmock
)

configure_target(linux-lib-test CONSOLE)
enable_release_link_optimizations(linux-lib-test)

set_target_properties(linux-lib-test PROPERTIES
    FOLDER "Tests"
)

gtest_discover_tests(linux-lib-test
    TEST_PREFIX "linux-lib."
    DISCOVERY_MODE POST_BUILD
    DISCOVERY_TIMEOUT 120
    PROPERTIES
        TIMEOUT "${VSHALYGIN_COMMON_TEST_TIMEOUT}"
)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES
    ${linux_lib_test_sources}
)
