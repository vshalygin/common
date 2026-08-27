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
)
add_library(vshalygin::win-lib ALIAS win-lib)

target_sources(win-lib PUBLIC
    FILE_SET public_headers
    TYPE HEADERS
    BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}"
    FILES ${win_lib_headers}
)

target_include_directories(win-lib PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

configure_target(win-lib LIBRARY)

set_target_properties(win-lib PROPERTIES
    DEBUG_POSTFIX d
    FOLDER "Libraries"
)

install(TARGETS win-lib
    EXPORT vshalygin-common-targets
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    FILE_SET public_headers DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES
    ${win_lib_sources}
    ${win_lib_headers}
)
