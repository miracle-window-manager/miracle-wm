cmake_minimum_required(VERSION 3.7)
cmake_policy(SET CMP0015 NEW)
cmake_policy(SET CMP0022 NEW)

project(miracle-wm)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

option(SNAP_BUILD "Building as a snap?" OFF)
option(SYSTEMD_INTEGRATION "Specifies that the systemd integration script will run at startup" OFF)

find_package(PkgConfig)
pkg_check_modules(MIRAL miral REQUIRED)
pkg_check_modules(MIROIL miroil REQUIRED)
pkg_check_modules(MIRRENDERER mirrenderer REQUIRED)
pkg_check_modules(MIRPLATFORM mirplatform REQUIRED)
pkg_check_modules(MIRCOMMON mircommon REQUIRED)
pkg_check_modules(MIRCOMMON_INTERNAL mircommon-internal REQUIRED)
pkg_check_modules(MIRSERVER mirserver REQUIRED)
pkg_check_modules(MIRSERVER_INTERNAL mirserver-internal REQUIRED)
pkg_check_modules(MIRWAYLAND mirwayland REQUIRED)
pkg_check_modules(GLIB REQUIRED IMPORTED_TARGET glib-2.0)
pkg_check_modules(YAML REQUIRED IMPORTED_TARGET yaml-cpp)
pkg_check_modules(LIBEVDEV REQUIRED IMPORTED_TARGET libevdev)
find_package(nlohmann_json 3.2.0 REQUIRED)
pkg_check_modules(EGL REQUIRED IMPORTED_TARGET egl)
pkg_check_modules(GLESv2 REQUIRED IMPORTED_TARGET glesv2)

include(GNUInstallDirs)

add_library(miracle-wm-implementation
    STATIC
    src/policy.cpp
    src/tiling_window_tree.cpp
    src/container.cpp
    src/window_helpers.h
    src/window_helpers.cpp
    src/config.cpp
    src/config.h
    src/output.cpp
    src/workspace_manager.cpp
    src/ipc.cpp
    src/auto_restarting_launcher.cpp
    src/workspace_observer.cpp
    src/workspace.cpp
    src/leaf_container.cpp
    src/parent_container.cpp
    src/window_manager_tools_window_controller.cpp
    src/renderer.cpp
    src/tessellation_helpers.cpp
    src/miracle_gl_config.cpp
    src/ipc_command.cpp
    src/ipc_command_executor.cpp
    src/surface_tracker.cpp
    src/window_tools_accessor.cpp
    src/animator.cpp
    src/animation_definition.cpp
    src/program_factory.cpp
    src/mode_observer.cpp
    src/debug_helper.h
    src/floating_window_container.cpp
    src/shell_component_container.cpp
    src/container_group_container.cpp
    src/container_group_container.h
    src/floating_tree_container.cpp
    src/floating_tree_container.h
    src/layout_scheme.cpp
    src/utility_general.h
    src/minimal_window_manager.cpp src/minimal_window_manager.h
    src/config_error_handler.cpp src/config_error_handler.h
    src/scratchpad.h src/scratchpad.cpp
    src/compositor_state.h src/compositor_state.cpp
    src/math_helpers.h
)

add_executable(miracle-wm
    src/main.cpp
)

target_include_directories(miracle-wm-implementation PUBLIC SYSTEM
    ${MIRAL_INCLUDE_DIRS}
    ${MIROIL_INCLUDE_DIRS}
    ${MIRCOMMON_INCLUDE_DIRS}
    ${MIRRENDERER_INCLUDE_DIRS}
    ${MIRPLATFORM_INCLUDE_DIRS}
    ${MIRCOMMON_INTERNAL_INCLUDE_DIRS}
    ${MIRSERVER_INTERNAL_INCLUDE_DIRS}
    ${MIRSERVER_INCLUDE_DIRS}
    ${PROJECT_SOURCE_DIR}/include)

target_link_libraries(miracle-wm-implementation
    ${MIRAL_LDFLAGS}
    ${MIROIL_LDFLAGS}
    ${MIRRENDERER_LDFLAGS}
    ${MIRPLATFORM_LDFLAGS}
    ${MIRSERVER_LDFLAGS}
    ${MIRSERVER_INTERNAL_LDFLAGS}
    ${MIRWAYLAND_LDFLAGS}
    PkgConfig::YAML
    PkgConfig::GLIB
    PkgConfig::LIBEVDEV
    nlohmann_json::nlohmann_json
    PkgConfig::EGL
    PkgConfig::GLESv2
    -lpcre2-8 -lpcre2-16 -lpcre2-32)

if(SYSTEMD_INTEGRATION)
    # Install systemd files
    install(FILES session/usr/lib/systemd/user/miracle-wm-session.target
        session/usr/lib/systemd/user/miracle-wm-session-shutdown.target
        DESTINATION /usr/lib/systemd/user/)

    # Install the setup script
    install(PROGRAMS session/usr/bin/libexec/miracle-wm-session-setup
        DESTINATION ${CMAKE_INSTALL_LIBEXECDIR})

    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/miracle-wm-session.systemd.in miracle-wm-session @ONLY)
else()
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/miracle-wm-session.none.in miracle-wm-session @ONLY)
endif()

target_include_directories(miracle-wm PUBLIC SYSTEM ${MIRAL_INCLUDE_DIRS})
target_link_libraries(miracle-wm PUBLIC ${MIRAL_LDFLAGS} PRIVATE miracle-wm-implementation)

install(PROGRAMS ${CMAKE_BINARY_DIR}/bin/miracle-wm
    DESTINATION ${CMAKE_INSTALL_BINDIR}
)

configure_file(session/usr/local/share/wayland-sessions/miracle-wm.desktop.in miracle-wm.desktop @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/miracle-wm.desktop
    DESTINATION ${CMAKE_INSTALL_DATADIR}/wayland-sessions)

add_custom_target(miracle-wm-sensible-terminal ALL
    cp ${CMAKE_CURRENT_SOURCE_DIR}/src/miracle-wm-sensible-terminal ${CMAKE_BINARY_DIR}/bin
)

install(PROGRAMS
    src/miracle-wm-sensible-terminal
    ${CMAKE_CURRENT_BINARY_DIR}/miracle-wm-session
    DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(SNAP_BUILD)
    add_custom_target(miracle-wm-unsnap ALL
        cp ${CMAKE_CURRENT_SOURCE_DIR}/src/miracle-wm-unsnap ${CMAKE_BINARY_DIR}/bin
    )
    install(PROGRAMS
        src/miracle-wm-unsnap
        DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()

add_subdirectory(tests/)
add_subdirectory(miraclemsg/)
