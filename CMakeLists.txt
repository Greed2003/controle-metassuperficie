cmake_minimum_required(VERSION 3.20)

project(controle_metassuperficie LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(metasurface_core STATIC
    src/mode.cpp
    src/controller_state.cpp
    src/binary_input.cpp
    src/mode_command.cpp
    src/command_processor.cpp
    src/line_receiver.cpp
    src/pattern_catalog.cpp
    src/table_command.cpp
    src/pattern_list.cpp
)
target_include_directories(metasurface_core PUBLIC include)
target_compile_options(metasurface_core PRIVATE -Wall -Wextra -Wpedantic)

include(CTest)

if(BUILD_TESTING)
    find_package(GTest REQUIRED)

    add_executable(mode_tests tests/mode_test.cpp)
    target_link_libraries(mode_tests PRIVATE metasurface_core GTest::gtest_main)
    target_compile_options(mode_tests PRIVATE -Wall -Wextra -Wpedantic)

    add_executable(controller_state_tests tests/controller_state_test.cpp)
    target_link_libraries(controller_state_tests PRIVATE metasurface_core GTest::gtest_main)
    target_compile_options(controller_state_tests PRIVATE -Wall -Wextra -Wpedantic)

    include(GoogleTest)
    gtest_discover_tests(mode_tests)
    gtest_discover_tests(controller_state_tests)

    add_executable(binary_input_tests tests/binary_input_test.cpp)
    target_link_libraries(binary_input_tests PRIVATE metasurface_core GTest::gtest_main)
    target_compile_options(binary_input_tests PRIVATE -Wall -Wextra -Wpedantic)
    gtest_discover_tests(binary_input_tests)

    add_executable(mode_command_tests tests/mode_command_test.cpp)
    target_link_libraries(mode_command_tests PRIVATE metasurface_core GTest::gtest_main)
    target_compile_options(mode_command_tests PRIVATE -Wall -Wextra -Wpedantic)
    gtest_discover_tests(mode_command_tests)

    add_executable(command_processor_tests tests/command_processor_test.cpp)
    target_link_libraries(command_processor_tests PRIVATE metasurface_core GTest::gtest_main)
    target_compile_options(command_processor_tests PRIVATE -Wall -Wextra -Wpedantic)
    gtest_discover_tests(command_processor_tests)

    add_executable(line_receiver_tests tests/line_receiver_test.cpp)
    target_link_libraries(line_receiver_tests PRIVATE metasurface_core GTest::gtest_main)
    target_compile_options(line_receiver_tests PRIVATE -Wall -Wextra -Wpedantic)
    gtest_discover_tests(line_receiver_tests)

    add_executable(pattern_catalog_tests tests/pattern_catalog_test.cpp)
    target_link_libraries(pattern_catalog_tests PRIVATE metasurface_core GTest::gtest_main)
    target_compile_options(pattern_catalog_tests PRIVATE -Wall -Wextra -Wpedantic)
    gtest_discover_tests(pattern_catalog_tests)

    add_executable(table_command_tests tests/table_command_test.cpp)
    target_link_libraries(table_command_tests PRIVATE metasurface_core GTest::gtest_main)
    target_compile_options(table_command_tests PRIVATE -Wall -Wextra -Wpedantic)
    gtest_discover_tests(table_command_tests)

    add_executable(list_command_tests tests/list_command_test.cpp)
    target_link_libraries(list_command_tests PRIVATE metasurface_core GTest::gtest_main)
    target_compile_options(list_command_tests PRIVATE -Wall -Wextra -Wpedantic)
    gtest_discover_tests(list_command_tests)
endif()
