cmake_minimum_required(VERSION 3.7)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# Fetch Google Test and Google Mock
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
)

FetchContent_MakeAvailable(googletest)

include_directories(
    ${PROJECT_SOURCE_DIR}/src
)

find_package(PkgConfig)
pkg_check_modules(MIRAL miral REQUIRED)
pkg_check_modules(MIRSERVER mirserver REQUIRED)
pkg_check_modules(YAML REQUIRED IMPORTED_TARGET yaml-cpp)

add_executable(miracle-wm-tests
    mock_configuration.h
    mock_container.h
    mock_output.h
    test_filesystem_configuration.cpp
    test_tiling_window_tree.cpp
    test_ipc_command_parser.cpp
    test_animator.cpp
    test_feature_flags.cpp
    test_drag_and_drop_service.cpp
    with_command_controller.h
    stub_configuration.h
    stub_session.h
    stub_surface.h
    stub_window_controller.h
    stub_container.h)

target_include_directories(miracle-wm-tests PUBLIC SYSTEM
    ${MIRAL_INCLUDE_DIRS}
    ${MIRSERVER_INCLUDE_DIRS})

target_link_libraries(miracle-wm-tests
    miracle-wm-implementation
    ${GTEST_LIBRARIES}
    ${MIRAL_LDFLAGS}
    ${MIRSERVER_LDFLAGS}
    PkgConfig::YAML
    pthread
    gmock gtest gtest_main)

enable_testing()

