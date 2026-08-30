cmake_minimum_required(VERSION 3.25...4.4)

project(vshalygin_common
    VERSION 0.1.0
    LANGUAGES CXX
)

if(NOT MSVC AND
   NOT CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND
   NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    message(FATAL_ERROR
        "The current CMake build supports MSVC, GCC, and Clang only."
    )
endif()

if(CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING
        "Supported build configurations" FORCE
    )
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "CMake")

include(GNUInstallDirs)
include(CTest)
include(cmake/ProjectOptions.cmake)
include(cmake/ExternalDependencies.cmake)

set(VSHALYGIN_COMMON_TEST_TIMEOUT "60" CACHE STRING
    "Timeout in seconds for each discovered test"
)
if(NOT VSHALYGIN_COMMON_TEST_TIMEOUT MATCHES "^[1-9][0-9]*$")
    message(FATAL_ERROR
        "VSHALYGIN_COMMON_TEST_TIMEOUT must be a positive integer."
    )
endif()

add_subdirectory(common-lib)
if(WIN32)
    add_subdirectory(win-lib)
endif()
add_subdirectory(rpc-lib)

if(BUILD_TESTING)
    include(GoogleTest)
    add_subdirectory(test/common-lib-test)
    if(WIN32)
        add_subdirectory(test/win-lib-test)
    endif()
    add_subdirectory(test/rpc-lib-test)

    add_coverage_report_target(
        common-lib-test
        rpc-lib-test
    )
endif()

include(CMakePackageConfigHelpers)

set(vshalygin_common_install_cmakedir
    "${CMAKE_INSTALL_LIBDIR}/cmake/vshalygin-common"
)

configure_package_config_file(
    cmake/vshalygin-common-config.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/vshalygin-common-config.cmake"
    INSTALL_DESTINATION "${vshalygin_common_install_cmakedir}"
)

install(EXPORT vshalygin-common-targets
    FILE vshalygin-common-targets.cmake
    NAMESPACE vshalygin::
    DESTINATION "${vshalygin_common_install_cmakedir}"
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/vshalygin-common-config.cmake"
    DESTINATION "${vshalygin_common_install_cmakedir}"
)
