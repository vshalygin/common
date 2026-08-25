function(configure_msvc_target target target_kind)
    if(NOT TARGET "${target}")
        message(FATAL_ERROR "Unknown target: ${target}")
    endif()

    if(target_kind STREQUAL "LIBRARY")
        set(standard_scope PUBLIC)
        set(target_definition _LIB)
    elseif(target_kind STREQUAL "CONSOLE")
        set(standard_scope PRIVATE)
        set(target_definition _CONSOLE)
    else()
        message(FATAL_ERROR
            "Unsupported target kind '${target_kind}' for target '${target}'"
        )
    endif()

    target_compile_features("${target}" ${standard_scope} cxx_std_17)

    set_target_properties("${target}" PROPERTIES
        C_STANDARD 17
        C_STANDARD_REQUIRED ON
        C_EXTENSIONS OFF
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
        INTERPROCEDURAL_OPTIMIZATION_RELEASE ON
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
endfunction()

function(enable_release_link_optimizations target)
    target_link_options("${target}" PRIVATE
        "$<$<CONFIG:Release>:/OPT:REF>"
        "$<$<CONFIG:Release>:/OPT:ICF>"
    )
endfunction()
