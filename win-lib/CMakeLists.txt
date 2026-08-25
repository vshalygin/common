set(win_lib_sources
    win-lib/types/iocp.cpp
)

set(win_lib_headers
    win-lib/types/handle.h
    win-lib/types/internal/handle-traits.h
    win-lib/types/internal/unique-handle.h
    win-lib/types/iocp.h
)

add_library(win-lib STATIC
    ${win_lib_sources}
    ${win_lib_headers}
)
add_library(vshalygin::win-lib ALIAS win-lib)

target_include_directories(win-lib PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

configure_target(win-lib LIBRARY)

set_target_properties(win-lib PROPERTIES
    FOLDER "Libraries"
)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES
    ${win_lib_sources}
    ${win_lib_headers}
)
