cmake_minimum_required(VERSION 3.25...4.4)

project(vshalygin_rpc_example
    VERSION 0.1.0
    LANGUAGES CXX
)

if(NOT MSVC AND
   NOT CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND
   NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    message(FATAL_ERROR
        "The RPC example supports MSVC, GCC, and Clang only."
    )
endif()

if(CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING
        "Supported build configurations" FORCE
    )
endif()

find_package(Boost 1.86 REQUIRED COMPONENTS algorithm thread)
find_package(Protobuf CONFIG REQUIRED)
find_package(vshalygin-common CONFIG REQUIRED)

if(NOT TARGET Boost::algorithm)
    message(FATAL_ERROR "Boost must provide Boost::algorithm.")
endif()

if(NOT TARGET protobuf::libprotobuf OR NOT TARGET protobuf::protoc)
    message(FATAL_ERROR
        "Protobuf must provide protobuf::libprotobuf and protobuf::protoc."
    )
endif()

function(add_protobuf_sources target proto_file)
    get_filename_component(proto_file "${proto_file}" ABSOLUTE)
    get_filename_component(proto_directory "${proto_file}" DIRECTORY)
    get_filename_component(proto_directory_name "${proto_directory}" NAME)
    get_filename_component(proto_name "${proto_file}" NAME_WE)

    set(generated_root "${CMAKE_CURRENT_BINARY_DIR}/generated")
    set(generated_directory "${generated_root}/${proto_directory_name}")
    set(generated_source "${generated_directory}/${proto_name}.pb.cc")
    set(generated_header "${generated_directory}/${proto_name}.pb.h")

    add_custom_command(
        OUTPUT
            "${generated_source}"
            "${generated_header}"
        COMMAND
            "${CMAKE_COMMAND}" -E make_directory "${generated_directory}"
        COMMAND
            "$<TARGET_FILE:protobuf::protoc>"
            "--cpp_out=${generated_directory}"
            "--proto_path=${proto_directory}"
            "${proto_file}"
        DEPENDS
            "${proto_file}"
            protobuf::protoc
        COMMENT "Generating C++ sources from ${proto_name}.proto"
        VERBATIM
    )

    set_source_files_properties(
        "${generated_source}"
        "${generated_header}"
        PROPERTIES GENERATED TRUE
    )
    target_sources("${target}" PRIVATE
        "${proto_file}"
        "${generated_source}"
        "${generated_header}"
    )
    target_include_directories("${target}" PRIVATE
        "${generated_directory}"
    )
    source_group("Generated\\Protobuf" FILES
        "${generated_source}"
        "${generated_header}"
    )
    set_source_files_properties(
        "${generated_source}"
        PROPERTIES COMPILE_OPTIONS
            "$<$<CXX_COMPILER_ID:MSVC>:/W0>;$<$<CXX_COMPILER_ID:GNU>:-w>;$<$<CXX_COMPILER_ID:Clang>:-w>"
    )
endfunction()

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
    vshalygin::rpc-lib
    Boost::algorithm
    protobuf::libprotobuf
)

add_protobuf_sources(
    rpc-example
    "${CMAKE_CURRENT_SOURCE_DIR}/src/proto/services.proto"
)

target_compile_features(rpc-example PRIVATE cxx_std_17)

set_target_properties(rpc-example PROPERTIES
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    VS_GLOBAL_CharacterSet "Unicode"
)

if(MSVC)
    target_compile_definitions(rpc-example PRIVATE
        _CONSOLE
        UNICODE
        _UNICODE
        "$<$<CONFIG:Debug>:_DEBUG>"
        "$<$<CONFIG:Release>:NDEBUG>"
    )
    target_compile_options(rpc-example PRIVATE
        /W4
        /WX
        /sdl
        /permissive-
        /Zc:__cplusplus
        "$<$<CONFIG:Release>:/Gy>"
        "$<$<CONFIG:Release>:/Oi>"
    )
    target_link_options(rpc-example PRIVATE
        /DEBUG
        /SUBSYSTEM:CONSOLE
        "$<$<CONFIG:Release>:/OPT:REF>"
        "$<$<CONFIG:Release>:/OPT:ICF>"
    )
elseif(CMAKE_CXX_COMPILER_ID MATCHES "^(GNU|Clang)$")
    target_compile_options(rpc-example PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Werror
        -Wno-unknown-pragmas
        "$<$<CONFIG:Release>:-ffunction-sections>"
        "$<$<CONFIG:Release>:-fdata-sections>"
    )
    target_link_options(rpc-example PRIVATE
        "$<$<CONFIG:Release>:-Wl,--gc-sections>"
    )
endif()

include(CheckIPOSupported)
check_ipo_supported(RESULT ipo_supported OUTPUT ipo_error)
if(ipo_supported)
    set_property(TARGET rpc-example
        PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE ON
    )
endif()

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES
    ${rpc_example_sources}
    ${rpc_example_headers}
)
