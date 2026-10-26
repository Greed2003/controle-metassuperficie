cmake_minimum_required(VERSION 3.20)

project(controle_metassuperficie LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(metasurface_core STATIC src/mode.cpp)
target_include_directories(metasurface_core PUBLIC include)
target_compile_options(metasurface_core PRIVATE -Wall -Wextra -Wpedantic)

include(CTest)

if(BUILD_TESTING)
    find_package(GTest REQUIRED)

    add_executable(mode_tests tests/mode_test.cpp)
    target_link_libraries(mode_tests PRIVATE metasurface_core GTest::gtest_main)
    target_compile_options(mode_tests PRIVATE -Wall -Wextra -Wpedantic)

    include(GoogleTest)
    gtest_discover_tests(mode_tests)
endif()
