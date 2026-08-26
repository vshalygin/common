include(CheckIPOSupported)

check_ipo_supported(RESULT IPO_SUPPORTED OUTPUT IPO_ERROR)

function(configure_target target target_kind)
    if(NOT TARGET "${target}")
        message(FATAL_ERROR "Unknown target: ${target}")
    endif()

    if(target_kind STREQUAL "LIBRARY")
        set(standard_scope PUBLIC)
    elseif(target_kind STREQUAL "CONSOLE")
        set(standard_scope PRIVATE)
    else()
        message(FATAL_ERROR
            "Unsupported target kind '${target_kind}' for target '${target}'"
        )
    endif()

    target_compile_features("${target}" ${standard_scope} cxx_std_17)

    set_target_properties("${target}" PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
    )

    if(IPO_SUPPORTED)
        set_property(TARGET "${target}"
            PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE ON
        )
    endif()

    if(MSVC)
        if(target_kind STREQUAL "LIBRARY")
            set(target_definition _LIB)
        else()
            set(target_definition _CONSOLE)
        endif()

        set_target_properties("${target}" PROPERTIES
            MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
            VS_GLOBAL_CharacterSet "Unicode"
        )

        target_compile_definitions("${target}" PRIVATE
            ${target_definition}
            UNICODE
            _UNICODE
            "$<$<CONFIG:Debug>:_DEBUG>"
            "$<$<CONFIG:Release>:NDEBUG>"
        )

        target_compile_options("${target}" PRIVATE
            /W4
            /WX
            /sdl
            /permissive-
            /Zc:__cplusplus
            "$<$<CONFIG:Release>:/Gy>"
            "$<$<CONFIG:Release>:/Oi>"
        )

        if(target_kind STREQUAL "CONSOLE")
            target_link_options("${target}" PRIVATE
                /DEBUG
                /SUBSYSTEM:CONSOLE
            )
        endif()
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "^(GNU|Clang)$")
        target_compile_options("${target}" PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Werror
            -Wno-unknown-pragmas
        )
    endif()
endfunction()

function(enable_release_link_optimizations target)
    if(MSVC)
        target_link_options("${target}" PRIVATE
            "$<$<CONFIG:Release>:/OPT:REF>"
            "$<$<CONFIG:Release>:/OPT:ICF>"
        )
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "^(GNU|Clang)$")
        target_compile_options("${target}" PRIVATE
            "$<$<CONFIG:Release>:-ffunction-sections>"
            "$<$<CONFIG:Release>:-fdata-sections>"
        )
        target_link_options("${target}" PRIVATE
            "$<$<CONFIG:Release>:-Wl,--gc-sections>"
        )
    endif()
endfunction()
