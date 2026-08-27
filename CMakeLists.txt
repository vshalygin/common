cmake_minimum_required(VERSION 3.25...4.4)

project(vshalygin_common
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

option(BUILD_EXAMPLES "Build the example applications" ON)

include(CTest)
include(cmake/ProjectOptions.cmake)
include(cmake/ExternalDependencies.cmake)

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
endif()

if(BUILD_EXAMPLES)
    add_subdirectory(example/rpc-example)
endif()
