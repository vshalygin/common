option(VSHALYGIN_COMMON_ENABLE_COVERAGE
    "Instrument project targets and enable the coverage report target"
    OFF
)

if(VSHALYGIN_COMMON_ENABLE_COVERAGE)
    if(NOT BUILD_TESTING)
        message(FATAL_ERROR
            "VSHALYGIN_COMMON_ENABLE_COVERAGE requires BUILD_TESTING=ON."
        )
    endif()

    if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        message(FATAL_ERROR
            "Coverage reporting currently supports GCC only."
        )
    endif()

    if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
        message(FATAL_ERROR
            "Coverage reporting requires a Debug build."
        )
    endif()

    find_program(VSHALYGIN_GCOVR_EXECUTABLE
        NAMES gcovr
        DOC "Path to the gcovr executable"
    )
    if(NOT VSHALYGIN_GCOVR_EXECUTABLE)
        message(FATAL_ERROR
            "gcovr is required for coverage reporting. "
            "Install gcovr 8.6 or newer and configure again."
        )
    endif()

    execute_process(
        COMMAND "${VSHALYGIN_GCOVR_EXECUTABLE}" --version
        RESULT_VARIABLE gcovr_result
        OUTPUT_VARIABLE gcovr_version_output
        ERROR_VARIABLE gcovr_version_error
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(NOT gcovr_result EQUAL 0 OR
       NOT gcovr_version_output MATCHES "gcovr ([0-9]+\\.[0-9]+(\\.[0-9]+)?)")
        message(FATAL_ERROR
            "Unable to determine the gcovr version: "
            "${gcovr_version_error}"
        )
    endif()

    set(gcovr_version "${CMAKE_MATCH_1}")
    if(gcovr_version VERSION_LESS "8.6")
        message(FATAL_ERROR
            "gcovr 8.6 or newer is required; found ${gcovr_version}."
        )
    endif()
endif()

function(enable_target_coverage target)
    if(NOT VSHALYGIN_COMMON_ENABLE_COVERAGE)
        return()
    endif()

    target_compile_options("${target}" PRIVATE
        --coverage
        -O0
        -g
        -fprofile-abs-path
        -fprofile-update=atomic
    )

    get_target_property(target_type "${target}" TYPE)
    if(target_type STREQUAL "EXECUTABLE" OR
       target_type STREQUAL "SHARED_LIBRARY" OR
       target_type STREQUAL "MODULE_LIBRARY")
        target_link_options("${target}" PRIVATE --coverage)
    endif()
endfunction()

function(add_coverage_report_target)
    if(NOT VSHALYGIN_COMMON_ENABLE_COVERAGE)
        return()
    endif()

    set(coverage_directory "${CMAKE_BINARY_DIR}/coverage")

    add_custom_target(coverage
        COMMAND
            "${CMAKE_COMMAND}" -E make_directory "${coverage_directory}"
        COMMAND
            "${CMAKE_COMMAND}"
            "-DCOVERAGE_BINARY_DIR=${CMAKE_BINARY_DIR}"
            -P "${CMAKE_SOURCE_DIR}/cmake/CleanCoverageData.cmake"
        COMMAND
            "${CMAKE_CTEST_COMMAND}"
            --test-dir "${CMAKE_BINARY_DIR}"
            --verbose
        COMMAND
            "${VSHALYGIN_GCOVR_EXECUTABLE}"
            --root "${CMAKE_SOURCE_DIR}"
            --object-directory "${CMAKE_BINARY_DIR}"
            --filter "${CMAKE_SOURCE_DIR}/common-lib/"
            --filter "${CMAKE_SOURCE_DIR}/rpc-lib/"
            --html-details "${coverage_directory}/index.html"
            --cobertura "${coverage_directory}/coverage.xml"
            --cobertura-pretty
            --txt "${coverage_directory}/coverage.txt"
            --markdown-summary "${coverage_directory}/coverage-summary.md"
            --print-summary
        DEPENDS ${ARGN}
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        COMMENT "Running tests and generating line and branch coverage reports"
        USES_TERMINAL
        VERBATIM
    )
endfunction()
