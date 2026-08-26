set(win_lib_test_sources
    src/main.cpp
    src/types/unique-handle-test.cpp
)

add_executable(win-lib-test
    ${win_lib_test_sources}
)

target_include_directories(win-lib-test PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)
target_link_libraries(win-lib-test PRIVATE
    win-lib
    GTest::gtest
    GTest::gmock
)

configure_target(win-lib-test CONSOLE)

set_target_properties(win-lib-test PROPERTIES
    FOLDER "Tests"
)

gtest_discover_tests(win-lib-test
    TEST_PREFIX "win-lib."
    DISCOVERY_TIMEOUT 30
    PROPERTIES
        TIMEOUT 60
)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES
    ${win_lib_test_sources}
)
