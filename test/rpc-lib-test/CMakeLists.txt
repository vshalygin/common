set(rpc_lib_test_sources
    src/channel/channel-test.cpp
    src/client-endpoint-test.cpp
    src/closure-guard/closure-guard-test.cpp
    src/connection/connection-watcher-test.cpp
    src/connection/connection-test.cpp
    src/connector/client-connector-test.cpp
    src/connector/service-connector-test.cpp
    src/controller/request-controller-test.cpp
    src/controller/response-controller-test.cpp
    src/endpoint/endpoint-test.cpp
    src/main.cpp
    src/pipe/memory-pipe/mem-buffer-test.cpp
    src/pipe/memory-pipe/mem-buffers-test.cpp
    src/pipe/memory-pipe/mem-pipe-env-test.cpp
    src/pipe/memory-pipe/mem-pipe-endpoint-test.cpp
    src/pipe/tcp-pipe/tcp-pipe-client-env-test.cpp
    src/pipe/tcp-pipe/tcp-pipe-endpoint-test.cpp
    src/pipe/tcp-pipe/tcp-pipe-server-env-test.cpp
    src/server-endpoint-test.cpp
    src/service/service-test.cpp
    src/transfer-message/transfer-message-test.cpp
    src/transport/transport-test.cpp
    src/types/request-result-test.cpp
    src/types/response-result-test.cpp
)

if(WIN32)
    list(APPEND rpc_lib_test_sources
        src/pipe/win-pipe/win-pipe-client-env-test.cpp
        src/pipe/win-pipe/win-pipe-endpoint-test.cpp
        src/pipe/win-pipe/win-pipe-iocp-owner-test.cpp
        src/pipe/win-pipe/win-pipe-server-env-test.cpp
        src/pipe/win-pipe/win-pipe-operations/win-pipe-create-operation-test.cpp
        src/pipe/win-pipe/win-pipe-operations/win-pipe-open-operation-test.cpp
        src/pipe/win-pipe/win-pipe-operations/win-pipe-read-operation-test.cpp
        src/pipe/win-pipe/win-pipe-operations/win-pipe-write-operation-test.cpp
    )
endif()

set(rpc_lib_test_headers
    src/mocks/authenticator-mock.h
    src/mocks/closure-mock.h
    src/mocks/connection-mock.h
    src/mocks/service-mock.h
)

add_executable(rpc-lib-test
    ${rpc_lib_test_sources}
    ${rpc_lib_test_headers}
)

target_include_directories(rpc-lib-test PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)
target_link_libraries(rpc-lib-test PRIVATE
    rpc-lib
    Boost::headers
    Boost::thread
    protobuf::libprotobuf
    GTest::gtest
    GTest::gmock
)

add_protobuf_sources(
    rpc-lib-test
    "${CMAKE_CURRENT_SOURCE_DIR}/src/proto/test-messages.proto"
)
configure_target(rpc-lib-test CONSOLE)
enable_release_link_optimizations(rpc-lib-test)

if(MSVC)
    target_compile_options(rpc-lib-test PRIVATE /MP)
endif()

set_target_properties(rpc-lib-test PROPERTIES
    FOLDER "Tests"
)

gtest_discover_tests(rpc-lib-test
    TEST_PREFIX "rpc-lib."
    DISCOVERY_TIMEOUT 30
    PROPERTIES
        TIMEOUT 60
)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES
    ${rpc_lib_test_sources}
    ${rpc_lib_test_headers}
)
