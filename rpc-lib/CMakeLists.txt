set(rpc_lib_sources
    rpc-lib/authenticator/simple-authenticator/simple-authenticator.cpp
    rpc-lib/closure-guard/closure-guard.cpp
    rpc-lib/internal/channel/channel.cpp
    rpc-lib/internal/connection/connection-watcher.cpp
    rpc-lib/internal/connection/connection.cpp
    rpc-lib/internal/connector/client-connector.cpp
    rpc-lib/internal/connector/server-connector.cpp
    rpc-lib/internal/controller/controller-cast.cpp
    rpc-lib/internal/transfer-message/transfer-message.cpp
    rpc-lib/internal/transport/transport.cpp
    rpc-lib/pipe/memory-pipe/mem-buffer.cpp
    rpc-lib/pipe/memory-pipe/mem-buffers.cpp
    rpc-lib/pipe/memory-pipe/mem-pipe-endpoint.cpp
    rpc-lib/pipe/memory-pipe/mem-pipe-env.cpp
    rpc-lib/pipe/tcp-pipe/tcp-pipe-client-env.cpp
    rpc-lib/pipe/tcp-pipe/tcp-pipe-endpoint.cpp
    rpc-lib/pipe/tcp-pipe/tcp-pipe-server-env.cpp
    rpc-lib/pipe/win-pipe/win-pipe-iocp-owner.cpp
    rpc-lib/pipe/win-pipe/win-pipe-endpoint.cpp
    rpc-lib/pipe/win-pipe/win-pipe-operations/win-pipe-create-operation.cpp
    rpc-lib/pipe/win-pipe/win-pipe-operations/win-pipe-open-operation.cpp
    rpc-lib/pipe/win-pipe/win-pipe-operations/win-pipe-read-operation.cpp
    rpc-lib/pipe/win-pipe/win-pipe-operations/win-pipe-write-operation.cpp
    rpc-lib/pipe/win-pipe/win-pipe-server-env.cpp
    rpc-lib/pipe/win-pipe/win-pipe-client-env.cpp
)

set(rpc_lib_headers
    rpc-lib/authenticator/iauthenticator.h
    rpc-lib/authenticator/simple-authenticator/simple-authenticator.h
    rpc-lib/client-endpoint.h
    rpc-lib/closure-guard.h
    rpc-lib/closure-guard/closure-guard.h
    rpc-lib/consts.h
    rpc-lib/internal/channel/channel.h
    rpc-lib/internal/connection/connection-watcher.h
    rpc-lib/internal/connection/connection.h
    rpc-lib/internal/connection/iconnection.h
    rpc-lib/internal/connector/client-connector.h
    rpc-lib/internal/connector/server-connector.h
    rpc-lib/internal/controller/irequest-controller.h
    rpc-lib/internal/controller/null-rpc-controller.h
    rpc-lib/internal/controller/request-controller-base.h
    rpc-lib/internal/controller/request-controller.h
    rpc-lib/internal/controller/response-controller-base.h
    rpc-lib/internal/controller/response-controller.h
    rpc-lib/internal/endpoint/endpoint.h
    rpc-lib/internal/service/iservice.h
    rpc-lib/internal/service/service.h
    rpc-lib/internal/transfer-message/transfer-message.h
    rpc-lib/internal/transport/transport.h
    rpc-lib/iresponse-controller.h
    rpc-lib/pipe/iclient-pipe-env.h
    rpc-lib/pipe/ipipe-endpoint.h
    rpc-lib/pipe/iserver-pipe-env.h
    rpc-lib/pipe/memory-pipe/mem-buffer.h
    rpc-lib/pipe/memory-pipe/mem-buffers.h
    rpc-lib/pipe/memory-pipe/mem-pipe-endpoint.h
    rpc-lib/pipe/memory-pipe/mem-pipe-env.h
    rpc-lib/pipe/pipe-op-res.h
    rpc-lib/pipe/pipe-wait-res.h
    rpc-lib/pipe/tcp-pipe/tcp-pipe-endpoint.h
    rpc-lib/pipe/tcp-pipe/tcp-pipe-server-env.h
    rpc-lib/pipe/tcp-pipe/tcp-pipe-client-env.h
    rpc-lib/pipe/win-pipe/win-pipe-iocp-owner.h
    rpc-lib/pipe/win-pipe/win-pipe-client-env.h
    rpc-lib/pipe/win-pipe/win-pipe-endpoint.h
    rpc-lib/pipe/win-pipe/win-pipe-operations/win-pipe-create-operation.h
    rpc-lib/pipe/win-pipe/win-pipe-operations/win-pipe-open-operation.h
    rpc-lib/pipe/win-pipe/win-pipe-operations/win-pipe-operation.h
    rpc-lib/pipe/win-pipe/win-pipe-operations/win-pipe-read-operation.h
    rpc-lib/pipe/win-pipe/win-pipe-operations/win-pipe-write-operation.h
    rpc-lib/pipe/win-pipe/win-pipe-server-env.h
    rpc-lib/server-endpoint.h
    rpc-lib/types.h
    rpc-lib/types/config.h
    rpc-lib/types/request-result.h
    rpc-lib/types/response-result.h
)

add_library(rpc-lib STATIC
    ${rpc_lib_sources}
    ${rpc_lib_headers}
)
add_library(vshalygin::rpc-lib ALIAS rpc-lib)

target_include_directories(rpc-lib PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
)
target_link_libraries(rpc-lib PUBLIC
    common-lib
    win-lib
    sdk::boost
    sdk::protobuf
    sdk::absl
)

configure_msvc_target(rpc-lib LIBRARY)

set_target_properties(rpc-lib PROPERTIES
    FOLDER "Libraries"
)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES
    ${rpc_lib_sources}
    ${rpc_lib_headers}
)
