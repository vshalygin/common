set(VSHALYGIN_COMMON_SANITIZER "none" CACHE STRING
    "Runtime sanitizer: none, address-undefined, or thread"
)
set_property(CACHE VSHALYGIN_COMMON_SANITIZER PROPERTY STRINGS
    none
    address-undefined
    thread
)

set(_vshalygin_common_supported_sanitizers
    none
    address-undefined
    thread
)
if(NOT VSHALYGIN_COMMON_SANITIZER IN_LIST
   _vshalygin_common_supported_sanitizers)
    message(FATAL_ERROR
        "Unsupported sanitizer '${VSHALYGIN_COMMON_SANITIZER}'. "
        "Supported values: ${_vshalygin_common_supported_sanitizers}."
    )
endif()
unset(_vshalygin_common_supported_sanitizers)

if(NOT VSHALYGIN_COMMON_SANITIZER STREQUAL "none")
    if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        message(FATAL_ERROR
            "Sanitizer builds currently require Clang."
        )
    endif()

    if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
        message(FATAL_ERROR
            "Sanitizer builds currently support Linux only."
        )
    endif()

    if(VSHALYGIN_COMMON_ENABLE_COVERAGE)
        message(FATAL_ERROR
            "Coverage and sanitizer instrumentation cannot be enabled "
            "in the same build."
        )
    endif()

    if(VSHALYGIN_COMMON_SANITIZER STREQUAL "thread" AND
       CMAKE_SIZEOF_VOID_P EQUAL 4)
        message(FATAL_ERROR
            "ThreadSanitizer requires a 64-bit build."
        )
    endif()
endif()

function(enable_target_sanitizers target)
    if(VSHALYGIN_COMMON_SANITIZER STREQUAL "none")
        return()
    endif()

    if(VSHALYGIN_COMMON_SANITIZER STREQUAL "address-undefined")
        set(sanitizer_flag -fsanitize=address,undefined)
    elseif(VSHALYGIN_COMMON_SANITIZER STREQUAL "thread")
        set(sanitizer_flag -fsanitize=thread)
    endif()

    target_compile_options("${target}" PRIVATE
        "${sanitizer_flag}"
        -O1
        -g
        -fno-omit-frame-pointer
        -fno-optimize-sibling-calls
    )

    get_target_property(target_type "${target}" TYPE)
    if(target_type STREQUAL "EXECUTABLE" OR
       target_type STREQUAL "SHARED_LIBRARY" OR
       target_type STREQUAL "MODULE_LIBRARY")
        target_link_options("${target}" PRIVATE "${sanitizer_flag}")
    endif()
endfunction()
