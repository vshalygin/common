find_package(Threads REQUIRED)

set(GOOGLETEST_ROOT
    "${PROJECT_SOURCE_DIR}/submodules/googletest"
    CACHE PATH "Root of the GoogleTest source tree"
)

set(_required_common_paths
    "${GOOGLETEST_ROOT}/googlemock/src/gmock-all.cc"
    "${GOOGLETEST_ROOT}/googletest/src/gtest-all.cc"
)

foreach(required_path IN LISTS _required_common_paths)
    if(NOT EXISTS "${required_path}")
        message(FATAL_ERROR
            "Required dependency path does not exist: ${required_path}\n"
            "Initialize the corresponding submodule or set its cache variable."
        )
    endif()
endforeach()

if(MSVC)
    set(EXTERNAL_SDK "$ENV{EXTERNAL_SDK}" CACHE PATH
        "Root of the prebuilt external SDK"
    )

    if(EXTERNAL_SDK STREQUAL "")
        message(FATAL_ERROR
            "EXTERNAL_SDK is not set. Set the environment variable or pass "
            "-DEXTERNAL_SDK=<path>."
        )
    endif()
    set(BOOST_ROOT
        "${EXTERNAL_SDK}/Boost/boost_1_86_0_s"
        CACHE PATH "Root of the prebuilt Boost 1.86 SDK"
    )
    set(PROTOBUF_ROOT
        "${EXTERNAL_SDK}/Protobuf/protobuf-33.0"
        CACHE PATH "Root of the prebuilt Protobuf 33.0 SDK"
    )
    set(ABSL_ROOT
        "${EXTERNAL_SDK}/absl"
        CACHE PATH "Root of the prebuilt Abseil SDK"
    )
    set(PROTOC_EXECUTABLE
        "${PROTOBUF_ROOT}/protoc.exe"
        CACHE FILEPATH "Path to the protoc executable"
    )

    set(_required_sdk_paths
        "${BOOST_ROOT}/include"
        "${BOOST_ROOT}/x64"
        "${PROTOBUF_ROOT}/src"
        "${PROTOBUF_ROOT}/lib"
        "${PROTOBUF_ROOT}/lib/libutf8_debug"
        "${PROTOBUF_ROOT}/lib/libutf8_release"
        "${ABSL_ROOT}/src"
        "${ABSL_ROOT}/lib/debug"
        "${ABSL_ROOT}/lib/release"
        "${PROTOC_EXECUTABLE}"
    )

    foreach(required_path IN LISTS _required_sdk_paths)
        if(NOT EXISTS "${required_path}")
            message(FATAL_ERROR
                "Required dependency path does not exist: ${required_path}\n"
                "Set EXTERNAL_SDK or the corresponding dependency cache variable."
            )
        endif()
    endforeach()

add_library(sdk-boost INTERFACE)
add_library(sdk::boost ALIAS sdk-boost)
target_include_directories(sdk-boost INTERFACE
    "${BOOST_ROOT}/include"
)
target_link_directories(sdk-boost INTERFACE
    "${BOOST_ROOT}/x64"
)

add_library(sdk-protobuf INTERFACE)
add_library(sdk::protobuf ALIAS sdk-protobuf)
target_include_directories(sdk-protobuf INTERFACE
    "${PROTOBUF_ROOT}/src"
)
target_link_directories(sdk-protobuf INTERFACE
    "${PROTOBUF_ROOT}/lib"
    "$<$<CONFIG:Debug>:${PROTOBUF_ROOT}/lib/libutf8_debug>"
    "$<$<CONFIG:Release>:${PROTOBUF_ROOT}/lib/libutf8_release>"
)
target_link_libraries(sdk-protobuf INTERFACE
    "$<$<CONFIG:Debug>:libprotobufd.lib>"
    "$<$<CONFIG:Release>:libprotobuf.lib>"
    libutf8_range.lib
    libutf8_validity.lib
)

set(_absl_libraries
    absl_base.lib
    absl_city.lib
    absl_civil_time.lib
    absl_cord.lib
    absl_cord_internal.lib
    absl_cordz_functions.lib
    absl_cordz_handle.lib
    absl_cordz_info.lib
    absl_cordz_sample_token.lib
    absl_crc_cord_state.lib
    absl_crc_cpu_detect.lib
    absl_crc_internal.lib
    absl_crc32c.lib
    absl_debugging_internal.lib
    absl_decode_rust_punycode.lib
    absl_demangle_internal.lib
    absl_demangle_rust.lib
    absl_die_if_null.lib
    absl_examine_stack.lib
    absl_exponential_biased.lib
    absl_failure_signal_handler.lib
    absl_flags_commandlineflag.lib
    absl_flags_commandlineflag_internal.lib
    absl_flags_config.lib
    absl_flags_internal.lib
    absl_flags_marshalling.lib
    absl_flags_parse.lib
    absl_flags_private_handle_accessor.lib
    absl_flags_program_name.lib
    absl_flags_reflection.lib
    absl_flags_usage.lib
    absl_flags_usage_internal.lib
    absl_graphcycles_internal.lib
    absl_hash.lib
    absl_hashtablez_sampler.lib
    absl_int128.lib
    absl_kernel_timeout_internal.lib
    absl_leak_check.lib
    absl_log_flags.lib
    absl_log_globals.lib
    absl_log_initialize.lib
    absl_log_internal_check_op.lib
    absl_log_internal_conditions.lib
    absl_log_internal_fnmatch.lib
    absl_log_internal_format.lib
    absl_log_internal_globals.lib
    absl_log_internal_log_sink_set.lib
    absl_log_internal_message.lib
    absl_log_internal_nullguard.lib
    absl_log_internal_proto.lib
    absl_log_internal_structured_proto.lib
    absl_log_severity.lib
    absl_log_sink.lib
    absl_low_level_hash.lib
    absl_malloc_internal.lib
    absl_periodic_sampler.lib
    absl_poison.lib
    absl_random_distributions.lib
    absl_random_internal_distribution_test_util.lib
    absl_random_internal_entropy_pool.lib
    absl_random_internal_platform.lib
    absl_random_internal_randen.lib
    absl_random_internal_randen_hwaes.lib
    absl_random_internal_randen_hwaes_impl.lib
    absl_random_internal_randen_slow.lib
    absl_random_internal_seed_material.lib
    absl_random_seed_gen_exception.lib
    absl_random_seed_sequences.lib
    absl_raw_hash_set.lib
    absl_raw_logging_internal.lib
    absl_scoped_set_env.lib
    absl_spinlock_wait.lib
    absl_stacktrace.lib
    absl_status.lib
    absl_statusor.lib
    absl_str_format_internal.lib
    absl_strerror.lib
    absl_string_view.lib
    absl_strings.lib
    absl_strings_internal.lib
    absl_symbolize.lib
    absl_synchronization.lib
    absl_throw_delegate.lib
    absl_time.lib
    absl_time_zone.lib
    absl_tracing_internal.lib
    absl_utf8_for_code_point.lib
    absl_vlog_config_internal.lib
)

add_library(sdk-absl INTERFACE)
add_library(sdk::absl ALIAS sdk-absl)
target_include_directories(sdk-absl INTERFACE
    "${ABSL_ROOT}/src"
)
target_link_directories(sdk-absl INTERFACE
    "$<$<CONFIG:Debug>:${ABSL_ROOT}/lib/debug>"
    "$<$<CONFIG:Release>:${ABSL_ROOT}/lib/release>"
)
target_link_libraries(sdk-absl INTERFACE
    ${_absl_libraries}
)

elseif(CMAKE_CXX_COMPILER_ID MATCHES "^(GNU|Clang)$")
    find_package(Boost 1.74 REQUIRED COMPONENTS thread)
    find_package(Protobuf REQUIRED)

    if(NOT Protobuf_PROTOC_EXECUTABLE)
        message(FATAL_ERROR
            "The protobuf compiler was not found. Install protobuf-compiler."
        )
    endif()

    set(PROTOC_EXECUTABLE
        "${Protobuf_PROTOC_EXECUTABLE}"
        CACHE FILEPATH "Path to the protoc executable" FORCE
    )

    add_library(sdk-boost INTERFACE)
    add_library(sdk::boost ALIAS sdk-boost)
    if(TARGET Boost::headers)
        target_link_libraries(sdk-boost INTERFACE Boost::headers)
    else()
        target_link_libraries(sdk-boost INTERFACE Boost::boost)
    endif()
    target_link_libraries(sdk-boost INTERFACE Boost::thread)

    add_library(sdk-protobuf INTERFACE)
    add_library(sdk::protobuf ALIAS sdk-protobuf)
    target_link_libraries(sdk-protobuf INTERFACE protobuf::libprotobuf)

    # New Protobuf releases use Abseil and expose it transitively through
    # protobuf::libprotobuf. Older distro releases do not require it.
    add_library(sdk-absl INTERFACE)
    add_library(sdk::absl ALIAS sdk-absl)
endif()

function(add_googletest_sources target)
    set(googletest_sources
        "${GOOGLETEST_ROOT}/googlemock/src/gmock-all.cc"
        "${GOOGLETEST_ROOT}/googletest/src/gtest-all.cc"
    )

    target_sources("${target}" PRIVATE ${googletest_sources})
    target_include_directories("${target}" SYSTEM PRIVATE
        "${GOOGLETEST_ROOT}/googletest"
        "${GOOGLETEST_ROOT}/googlemock"
        "${GOOGLETEST_ROOT}/googletest/include"
        "${GOOGLETEST_ROOT}/googlemock/include"
    )
    if(CMAKE_CXX_COMPILER_ID MATCHES "^(GNU|Clang)$")
        set_source_files_properties(
            ${googletest_sources}
            PROPERTIES COMPILE_OPTIONS -w
        )
    endif()
endfunction()

function(add_protobuf_sources target proto_file)
    get_filename_component(proto_file "${proto_file}" ABSOLUTE)
    get_filename_component(proto_directory "${proto_file}" DIRECTORY)
    get_filename_component(proto_name "${proto_file}" NAME_WE)

    set(generated_source "${proto_directory}/${proto_name}.pb.cc")
    set(generated_header "${proto_directory}/${proto_name}.pb.h")

    add_custom_command(
        OUTPUT
            "${generated_source}"
            "${generated_header}"
        COMMAND
            "${PROTOC_EXECUTABLE}"
            "--cpp_out=${proto_directory}"
            "--proto_path=${proto_directory}"
            "${proto_file}"
        DEPENDS
            "${proto_file}"
            "${PROTOC_EXECUTABLE}"
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
    set_source_files_properties(
        "${generated_source}"
        PROPERTIES COMPILE_OPTIONS
            "$<$<CXX_COMPILER_ID:MSVC>:/W0>;$<$<CXX_COMPILER_ID:GNU>:-w>;$<$<CXX_COMPILER_ID:Clang>:-w>"
    )
endfunction()
