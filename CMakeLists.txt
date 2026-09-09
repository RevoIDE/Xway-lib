cmake_minimum_required(VERSION 3.20)

project(xway-lib C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(xway STATIC
	src/xway.c
	src/wayland.c
	src/xdg-shell-protocol.c
)

add_executable(xway-main src/main.c)

target_link_libraries(xway-main PRIVATE xway)

target_include_directories(xway PUBLIC
	include
)

find_package(PkgConfig REQUIRED)
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
