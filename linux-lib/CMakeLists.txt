set(linux_lib_sources
    linux-lib/types/epoll.cpp
)

set(linux_lib_headers
    linux-lib/types/epoll.h
    linux-lib/types/unique-fd.h
)

add_library(linux-lib STATIC
    ${linux_lib_sources}
)
add_library(vshalygin::linux-lib ALIAS linux-lib)

target_sources(linux-lib PUBLIC
    FILE_SET public_headers
    TYPE HEADERS
    BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}"
    FILES ${linux_lib_headers}
)

target_include_directories(linux-lib PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

configure_target(linux-lib LIBRARY)

set_target_properties(linux-lib PROPERTIES
    DEBUG_POSTFIX d
    FOLDER "Libraries"
)

install(TARGETS linux-lib
    EXPORT vshalygin-common-targets
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    FILE_SET public_headers DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES
    ${linux_lib_sources}
    ${linux_lib_headers}
)
