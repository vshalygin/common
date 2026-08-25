set(rpc_example_sources
    src/app/app.cpp
    src/main.cpp
    src/utils/utils.cpp
)

set(rpc_example_headers
    src/app/app.h
    src/client/client-service.h
    src/client/client.h
    src/server/server-service.h
    src/server/server.h
    src/utils/utils.h
)

add_executable(rpc-example
    ${rpc_example_sources}
    ${rpc_example_headers}
)

target_link_libraries(rpc-example PRIVATE
    common-lib
    rpc-lib
    sdk::boost
    sdk::protobuf
    sdk::absl
)

add_protobuf_sources(
    rpc-example
    "${CMAKE_CURRENT_SOURCE_DIR}/src/proto/services.proto"
)
configure_target(rpc-example CONSOLE)

set_target_properties(rpc-example PROPERTIES
    FOLDER "Examples"
)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES
    ${rpc_example_sources}
    ${rpc_example_headers}
)
