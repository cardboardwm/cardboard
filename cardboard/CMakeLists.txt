cmake_minimum_required(VERSION 3.9)

project(cardboard)

set(CMAKE_CXX_STANDARD 17)

find_package(PkgConfig REQUIRED)
pkg_check_modules(wlroots REQUIRED wlroots)
pkg_check_modules(wayland-protocols REQUIRED wayland-protocols)
pkg_check_modules(wayland-scanner REQUIRED wayland-scanner)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*)
file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS include/*)

add_custom_target(
        xdg_shell_protocol_src ALL
        COMMAND wayland-scanner server-header /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml ${CMAKE_CURRENT_BINARY_DIR}/xdg-shell-protocol.h
        COMMAND wayland-scanner private-code /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml ${CMAKE_CURRENT_BINARY_DIR}/xdg-shell-protocol.c
        BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/xdg-shell-protocol.c
)

add_library(xdg_shell_protocol ${CMAKE_CURRENT_BINARY_DIR}/xdg-shell-protocol.c)
add_dependencies(xdg_shell_protocol xdg_shell_protocol_src)
target_include_directories(xdg_shell_protocol PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/)

add_executable(cardboard ${SOURCES})
target_link_libraries(cardboard ${wlroots_LIBRARIES} util xdg_shell_protocol)
target_include_directories(cardboard PRIVATE ${wlroots_INCLUDE_DIRS} include/)
target_compile_options(cardboard PRIVATE ${wlroots_CFLAGS_OTHER} "-DWLR_USE_UNSTABLE")
