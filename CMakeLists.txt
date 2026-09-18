cmake_minimum_required(VERSION 3.20)

project(xway-lib C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(PkgConfig REQUIRED)
find_program(WAYLAND_SCANNER wayland-scanner REQUIRED)

pkg_check_modules(WAYLAND_PROTOCOLS REQUIRED wayland-protocols)
pkg_check_modules(XKBCOMMON REQUIRED IMPORTED_TARGET xkbcommon)
pkg_get_variable(WAYLAND_PROTOCOLS_DIR wayland-protocols pkgdatadir)

set(XDG_SHELL_XML
    "${WAYLAND_PROTOCOLS_DIR}/stable/xdg-shell/xdg-shell.xml"
)

set(XDG_SHELL_C
    "${CMAKE_CURRENT_BINARY_DIR}/xdg-shell-protocol.c"
)

add_custom_command(
    OUTPUT "${XDG_SHELL_C}"
    COMMAND "${WAYLAND_SCANNER}" private-code
            "${XDG_SHELL_XML}" "${XDG_SHELL_C}"
    DEPENDS "${XDG_SHELL_XML}"
    COMMENT "Generation de xdg-shell-protocol.c"
    VERBATIM
)

  file(GLOB_RECURSE XWAY_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
  )

  list(REMOVE_ITEM XWAY_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/src/main.c"
  )

  add_library(xway STATIC
        ${XWAY_SOURCES}
        "${XDG_SHELL_C}"
  )

add_executable(xway-main src/main.c)

target_link_libraries(xway-main PRIVATE xway)

target_include_directories(xway PUBLIC
	include
)

pkg_check_modules(WAYLAND REQUIRED wayland-client)

target_include_directories(xway PRIVATE
    ${WAYLAND_INCLUDE_DIRS}
)

target_link_libraries(xway PRIVATE
    ${WAYLAND_LIBRARIES}
)

target_compile_options(xway PRIVATE
    ${WAYLAND_CFLAGS_OTHER}
)
target_link_libraries(xway PRIVATE PkgConfig::XKBCOMMON)
