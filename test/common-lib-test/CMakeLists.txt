set(common_lib_test_sources
    src/main.cpp
    src/memory/enable-shared-from-this-manual-set-test.cpp
    src/memory/unique-ptr2-test.cpp
    src/mpl/function-traits/function-traits-test.cpp
    src/mpl/function-traits/function-signature-test.cpp
    src/mpl/function-traits/is-function-pointer-test.cpp
    src/mpl/function-traits/is-std-function-test.cpp
    src/mpl/function-transform/make-function-type-test.cpp
    src/mpl/function-transform/remove-function-qualifiers-test.cpp
    src/mpl/function-transform/remove-member-function-qualifiers-test.cpp
    src/mpl/tuple-traits/is-std-tuple-test.cpp
    src/mpl/tuple-traits/tuple-element-test.cpp
    src/mpl/tuple-traits/tuple-size-test.cpp
    src/mpl/tuple-transform/mpl-merge-tuples-test.cpp
    src/mpl/tuple-transform/remove-last-tuple-element-test.cpp
    src/mpl/tuple-transform/mpl-sort-tuple-test.cpp
    src/mpl/tuple-transform/last-tuple-type-test.cpp
    src/mpl/tuple-transform/swap-first-two-tuple-types-test.cpp
    src/mpl/type-traits/check-type-test.cpp
    src/mpl/type-traits/is-lvalue-static-castable-test.cpp
    src/mpl/type-transform/add-const-test.cpp
    src/mpl/type-transform/add-lvalue-ref-to-value-test.cpp
    src/mpl/type-transform/remove-c-ref-test.cpp
    src/mpl/type-transform/remove-const-test.cpp
    src/mpl/type-transform/remove-type-qualifiers-test.cpp
    src/synchronization/ordered-lock/has-integral-order-static-member-test.cpp
    src/synchronization/ordered-lock/is-lockable-test.cpp
    src/synchronization/ordered-lock/is-there-repeating-order-number-test.cpp
    src/synchronization/ordered-lock/ordered-lock-test.cpp
    src/synchronization/ordered-lock/compare-ordered-lockable-tuples-test.cpp
    src/synchronization/ordered-mutex/ordered-mutex-test.cpp
    src/synchronization/value-locker-test.cpp
    src/thread/future/future-test.cpp
    src/thread/future/promise-test.cpp
    src/thread/thread-pool/strand-test.cpp
    src/thread/thread-pool/thread-pool-task-test.cpp
    src/thread/thread-pool/thread-pool-test.cpp
    src/timer/multiple-timer/multiple-timer-test.cpp
    src/timer/periodic-timer/periodic-timer-test.cpp
    src/utils/buffer-view/buffer-view-test.cpp
    src/utils/buffer-view/cbuffer-view-test.cpp
    src/utils/buffer/buffer-test.cpp
    src/utils/do-on-destruct/do-on-destruct-test.cpp
    src/utils/function/function-test.cpp
    src/utils/tuple-utils/extract-first-tuple-element-test.cpp
    src/utils/tuple-utils/extract-last-tuple-element-test.cpp
    src/utils/tuple-utils/for-each-tuple-element-reverse-test.cpp
    src/utils/tuple-utils/for-each-tuple-element-test.cpp
    src/utils/tuple-utils/forward-tuple-element.cpp
    src/utils/tuple-utils/merge-tuples-test.cpp
    src/utils/tuple-utils/sort-tuple-test.cpp
    src/utils/tuple-utils/sort-tuple2-test.cpp
    src/utils/tuple-utils/swap-first-two-tuple-elements-test.cpp
    src/utils/tuple-utils/tie-tuple-test.cpp
    src/utils/type-qualifiers-cast/type-qualifiers-cast-test.cpp
    src/utils/type-wrapper/type-wrapper-test.cpp
    src/utils/value-proxy/value-proxy-test.cpp
)

add_executable(common-lib-test
    ${common_lib_test_sources}
)

target_link_libraries(common-lib-test PRIVATE
    common-lib
    Boost::headers
    Boost::thread
    GTest::gtest
    GTest::gmock
)

configure_target(common-lib-test CONSOLE)
enable_release_link_optimizations(common-lib-test)

if(MSVC)
    set_source_files_properties(
        src/thread/future/future-test.cpp
        PROPERTIES COMPILE_OPTIONS /bigobj
    )
endif()

set_target_properties(common-lib-test PROPERTIES
    FOLDER "Tests"
)

gtest_discover_tests(common-lib-test
    TEST_PREFIX "common-lib."
    DISCOVERY_MODE POST_BUILD
    DISCOVERY_TIMEOUT 120
    PROPERTIES
        TIMEOUT 60
)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES
    ${common_lib_test_sources}
)
