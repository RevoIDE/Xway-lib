cmake_minimum_required(VERSION 3.20)

project(xway-lib C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(PkgConfig REQUIRED)
find_program(WAYLAND_SCANNER wayland-scanner REQUIRED)

pkg_check_modules(WAYLAND_PROTOCOLS REQUIRED wayland-protocols)
pkg_check_modules(XKBCOMMON REQUIRED IMPORTED_TARGET xkbcommon)
pkg_check_modules(WAYLAND_CURSOR REQUIRED IMPORTED_TARGET wayland-cursor)
pkg_get_variable(WAYLAND_PROTOCOLS_DIR wayland-protocols pkgdatadir)

set(XDG_SHELL_XML
    "${WAYLAND_PROTOCOLS_DIR}/stable/xdg-shell/xdg-shell.xml"
)

set(XDG_SHELL_C
    "${CMAKE_CURRENT_BINARY_DIR}/xdg-shell-protocol.c"
)
set(XDG_SHELL_HEADER
	"${CMAKE_CURRENT_BINARY_DIR}/xdg-shell-client-protocol.h"
)
set(RELATIVE_POINTER_HEADER
    "${CMAKE_CURRENT_BINARY_DIR}/relative-pointer-unstable-v1-client-protocol.h"
)

set(RELATIVE_POINTER_C
    "${CMAKE_CURRENT_BINARY_DIR}/relative-pointer-unstable-v1-protocol.c"
)

set(POINTER_CONSTRAINTS_HEADER
    "${CMAKE_CURRENT_BINARY_DIR}/pointer-constraints-unstable-v1-client-protocol.h"
)

set(POINTER_CONSTRAINTS_C
    "${CMAKE_CURRENT_BINARY_DIR}/pointer-constraints-unstable-v1-protocol.c"
)
set(RELATIVE_POINTER_XML
    "${WAYLAND_PROTOCOLS_DIR}/unstable/relative-pointer/relative-pointer-unstable-v1.xml"
)

set(POINTER_CONSTRAINTS_XML
    "${WAYLAND_PROTOCOLS_DIR}/unstable/pointer-constraints/pointer-constraints-unstable-v1.xml"
)

add_custom_command(
    OUTPUT
        "${XDG_SHELL_HEADER}"
        "${XDG_SHELL_C}"
    COMMAND
        "${WAYLAND_SCANNER}" client-header
        "${XDG_SHELL_XML}"
        "${XDG_SHELL_HEADER}"
    COMMAND
        "${WAYLAND_SCANNER}" private-code
        "${XDG_SHELL_XML}"
        "${XDG_SHELL_C}"
    DEPENDS
        "${XDG_SHELL_XML}"
    COMMENT "Generating xdg-shell protocol"
    VERBATIM
)

add_custom_command(
    OUTPUT
        "${RELATIVE_POINTER_HEADER}"
        "${RELATIVE_POINTER_C}"
    COMMAND
        "${WAYLAND_SCANNER}" client-header
        "${RELATIVE_POINTER_XML}"
        "${RELATIVE_POINTER_HEADER}"
    COMMAND
        "${WAYLAND_SCANNER}" private-code
        "${RELATIVE_POINTER_XML}"
        "${RELATIVE_POINTER_C}"
    DEPENDS
        "${RELATIVE_POINTER_XML}"
    COMMENT
        "Generating relative pointer protocol"
    VERBATIM
)

add_custom_command(
    OUTPUT
        "${POINTER_CONSTRAINTS_HEADER}"
        "${POINTER_CONSTRAINTS_C}"
    COMMAND
        "${WAYLAND_SCANNER}" client-header
        "${POINTER_CONSTRAINTS_XML}"
        "${POINTER_CONSTRAINTS_HEADER}"
    COMMAND
        "${WAYLAND_SCANNER}" private-code
        "${POINTER_CONSTRAINTS_XML}"
        "${POINTER_CONSTRAINTS_C}"
    DEPENDS
        "${POINTER_CONSTRAINTS_XML}"
    COMMENT
        "Generating pointer constraints protocol"
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
	  "${XDG_SHELL_HEADER}"
      "${XDG_SHELL_C}"
     "${RELATIVE_POINTER_C}"
  "${POINTER_CONSTRAINTS_C}"
)

add_executable(xway-main src/main.c)

target_link_libraries(xway-main PRIVATE xway)

target_include_directories(xway PUBLIC
	include
)

pkg_check_modules(WAYLAND REQUIRED wayland-client)

target_include_directories(xway PRIVATE
	"${CMAKE_CURRENT_BINARY_DIR}"
    ${WAYLAND_INCLUDE_DIRS}
)

target_link_libraries(xway PRIVATE
    ${WAYLAND_LIBRARIES}
)

target_compile_options(xway PRIVATE
    ${WAYLAND_CFLAGS_OTHER}
)
target_link_libraries(xway PRIVATE PkgConfig::XKBCOMMON)
target_link_libraries(xway PRIVATE PkgConfig::WAYLAND_CURSOR)
