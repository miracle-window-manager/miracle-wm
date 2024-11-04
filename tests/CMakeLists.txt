cmake_minimum_required(VERSION 3.7)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(
    ${PROJECT_SOURCE_DIR}/src
)

find_package(PkgConfig)
pkg_check_modules(MIRAL miral REQUIRED)
pkg_check_modules(MIRSERVER mirserver REQUIRED)
find_package(GTest REQUIRED)
pkg_check_modules(YAML REQUIRED IMPORTED_TARGET yaml-cpp)

add_executable(miracle-wm-tests
    filesystem_configuration_test.cpp
    tiling_window_tree_test.cpp
    test_i3_command.cpp
    test_animator.cpp
    stub_configuration.h
    stub_session.h
    stub_surface.h)

target_include_directories(miracle-wm-tests PUBLIC SYSTEM
        ${GTEST_INCLUDE_DIRS}
        ${MIRAL_INCLUDE_DIRS}
        ${MIRSERVER_INCLUDE_DIRS})
target_link_libraries(miracle-wm-tests
        GTest::gtest_main
        miracle-wm-implementation
        ${GTEST_LIBRARIES}
        ${MIRAL_LDFLAGS}
        ${MIRSERVER_LDFLAGS}
        PkgConfig::YAML
        pthread)
gtest_discover_tests(miracle-wm-tests)
