find_package(Threads REQUIRED)
find_package(Boost 1.86 REQUIRED COMPONENTS thread)
find_package(Protobuf CONFIG REQUIRED)

if(BUILD_TESTING)
    find_package(GTest CONFIG REQUIRED)
endif()

# CMake's FindBoost module historically exposes Boost::boost, while modern
# Boost config packages expose Boost::headers. Keep the project compatible
# with either package layout.
if(NOT TARGET Boost::headers AND TARGET Boost::boost)
    add_library(boost-headers-compat INTERFACE)
    target_link_libraries(boost-headers-compat INTERFACE Boost::boost)
    add_library(Boost::headers ALIAS boost-headers-compat)
endif()

if(NOT TARGET Boost::headers OR NOT TARGET Boost::thread)
    message(FATAL_ERROR
        "Boost 1.86 or newer must provide Boost::headers and Boost::thread."
    )
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
