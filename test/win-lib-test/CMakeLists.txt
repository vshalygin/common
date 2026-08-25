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
)

add_googletest_sources(win-lib-test)
configure_msvc_target(win-lib-test CONSOLE)

set_target_properties(win-lib-test PROPERTIES
    FOLDER "Tests"
)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES
    ${win_lib_test_sources}
)
