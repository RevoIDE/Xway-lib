cmake_minimum_required(VERSION 3.20)

project(xway-lib C)

set(CMAKE_C_STANDARD 11)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
	src/*.c
	include/*.h
)

add_executable(xway-lib ${SOURCES})

target_include_directories(xway-lib PRIVATE
	include
)

find_package(PkgConfig REQUIRED)
pkg_check_modules(WAYLAND REQUIRED wayland-client)

target_include_directories(xway-lib PRIVATE
    ${WAYLAND_INCLUDE_DIRS}
)

target_link_libraries(xway-lib PRIVATE
    ${WAYLAND_LIBRARIES}
)

target_compile_options(xway-lib PRIVATE
    ${WAYLAND_CFLAGS_OTHER}
)
