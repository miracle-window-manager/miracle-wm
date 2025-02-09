cmake_minimum_required(VERSION 3.7)
cmake_policy(SET CMP0015 NEW)
cmake_policy(SET CMP0022 NEW)

project(miracle-wm)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

option(SNAP_BUILD "Building as a snap?" OFF)
option(SYSTEMD_INTEGRATION "Specifies that the systemd integration script will run at startup" OFF)
add_compile_options(-Wimplicit-fallthrough)

add_compile_options(
    -Wall              # Enable most warnings
    -Wextra            # Additional warnings
    -Wpedantic         # Enforce strict compliance with the C++ standard
    # -Werror            # Treat warnings as errors
    -Wconversion       # Warn on type conversions that may lose data
    -Wsign-conversion  # Warn on implicit signed-unsigned conversions
    -Wimplicit-fallthrough  # Warn if switch-case fallthrough is not explicitly marked
    -Wunreachable-code # Warn about unreachable code
    -Wno-volatile      # Avoid warnings about volatile usage (C++23 deprecates some volatile features)
    -Wno-unused-parameter # Disable unused parameter warning
    #-fanalyzer         # Enable GCC static analyzer
    #-fmodules-ts       # Enable experimental modules (GCC/Clang)
)

if(CMAKE_BUILD_TYPE MATCHES Debug)
    add_compile_options(
        -g                         # Debug symbols
        #-fsanitize=address,undefined # Address and Undefined Behavior Sanitizer
        -fno-omit-frame-pointer     # Improve debugging accuracy
    )
    add_link_options(
        -fsanitize=address,undefined
    )
endif()


if(CMAKE_BUILD_TYPE MATCHES Release)
    add_compile_options(
        -O3           # Maximum optimization
        -march=native # Optimize for the host architecture
        -flto         # Link-time optimization
    )
endif()

find_package(PkgConfig)

set(MIR_MINIMUM_VERSION 2.18)

pkg_check_modules(MIRAL REQUIRED miral>=5.1)
pkg_check_modules(MIROIL REQUIRED miroil>=5.0)
pkg_check_modules(MIRRENDERER REQUIRED mirrenderer>=${MIR_MINIMUM_VERSION})
pkg_check_modules(MIRPLATFORM REQUIRED mirplatform>=${MIR_MINIMUM_VERSION})
pkg_check_modules(MIRCOMMON REQUIRED mircommon>=${MIR_MINIMUM_VERSION})
pkg_check_modules(MIRCOMMON_INTERNAL REQUIRED mircommon-internal>=${MIR_MINIMUM_VERSION})
pkg_check_modules(MIRSERVER REQUIRED mirserver>=${MIR_MINIMUM_VERSION})
pkg_check_modules(MIRSERVER_INTERNAL REQUIRED mirserver-internal>=${MIR_MINIMUM_VERSION})
pkg_check_modules(MIRWAYLAND REQUIRED mirwayland>=${MIR_MINIMUM_VERSION})
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
    src/container.cpp
    src/window_helpers.h
    src/window_helpers.cpp
    src/config.cpp
    src/config.h
    src/output.cpp
    src/workspace_manager.cpp
    src/ipc.cpp
    src/auto_restarting_launcher.cpp
    src/workspace_interface.h
    src/workspace_observer.cpp
    src/workspace.cpp src/workspace.h
    src/leaf_container.cpp
    src/parent_container.cpp
    src/window_manager_tools_window_controller.cpp
    src/renderer.cpp
    src/tessellation_helpers.cpp
    src/miracle_gl_config.cpp
    src/ipc_command.cpp
    src/ipc_command_executor.cpp
    src/render_data_manager.cpp
    src/animator.cpp
    src/animation_definition.cpp
    src/program_factory.cpp
    src/mode_observer.cpp
    src/debug_helper.h
    src/shell_component_container.cpp
    src/container_group_container.cpp
    src/container_group_container.h
    src/layout_scheme.cpp
    src/utility_general.h
    src/config_error_handler.cpp src/config_error_handler.h
    src/scratchpad.h src/scratchpad.cpp
    src/compositor_state.h src/compositor_state.cpp
    src/math_helpers.h
    src/command_controller.cpp src/command_controller.h
    src/animator_loop.cpp src/animator_loop.h
    src/drag_and_drop_service.cpp src/drag_and_drop_service.h
    src/output_factory_interface.h
    src/output_manager.cpp src/output_manager.h
    src/output_factory.cpp src/output_factory.cpp
    src/output.h src/output.cpp
    src/move_service.h src/move_service.cpp
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
