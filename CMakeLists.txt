cmake_minimum_required(VERSION 3.25...4.4)

project(common
    LANGUAGES CXX
)

if(NOT WIN32)
    message(FATAL_ERROR "The current CMake build supports Windows only.")
endif()

if(NOT MSVC)
    message(FATAL_ERROR "The current CMake build supports the MSVC compiler only.")
endif()

set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING
    "Supported build configurations" FORCE
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "CMake")

option(BUILD_TESTS "Build the existing test executables" ON)
option(BUILD_EXAMPLES "Build the example applications" ON)

include(cmake/ProjectOptions.cmake)
include(cmake/ExternalDependencies.cmake)

add_subdirectory(common-lib)
add_subdirectory(win-lib)
add_subdirectory(rpc-lib)

if(BUILD_TESTS)
    add_subdirectory(test/common-lib-test)
    add_subdirectory(test/win-lib-test)
    add_subdirectory(test/rpc-lib-test)
endif()

if(BUILD_EXAMPLES)
    add_subdirectory(example/rpc-example)
endif()
