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
    src/command_response.cpp
    src/output_control.cpp
    src/controller_application.cpp
)
target_include_directories(metasurface_core PUBLIC include)
target_compile_options(metasurface_core PRIVATE -Wall -Wextra -Wpedantic)

add_library(metasurface_avr STATIC
    src/avr_digital_output.cpp
    src/avr_uart.cpp
)
target_link_libraries(metasurface_avr PUBLIC metasurface_core)
target_compile_options(metasurface_avr PRIVATE -Wall -Wextra -Wpedantic)
target_compile_definitions(metasurface_avr PUBLIC F_CPU=16000000UL)

include(CTest)

if(CMAKE_CROSSCOMPILING AND BUILD_TESTING)
    message(FATAL_ERROR "Use -DBUILD_TESTING=OFF para AVR; execute os testes na compilacao nativa.")
endif()

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

    add_executable(command_response_tests tests/command_response_test.cpp)
    target_link_libraries(command_response_tests PRIVATE metasurface_core GTest::gtest_main)
    target_compile_options(command_response_tests PRIVATE -Wall -Wextra -Wpedantic)
    gtest_discover_tests(command_response_tests)

    add_executable(output_control_tests tests/output_control_test.cpp)
    target_link_libraries(output_control_tests PRIVATE metasurface_core GTest::gtest_main)
    target_compile_options(output_control_tests PRIVATE -Wall -Wextra -Wpedantic)
    gtest_discover_tests(output_control_tests)

    add_executable(avr_digital_output_tests tests/avr_digital_output_test.cpp)
    target_link_libraries(avr_digital_output_tests PRIVATE metasurface_avr GTest::gtest_main)
    target_compile_options(avr_digital_output_tests PRIVATE -Wall -Wextra -Wpedantic)
    gtest_discover_tests(avr_digital_output_tests)

    add_executable(avr_uart_tests tests/avr_uart_test.cpp)
    target_link_libraries(avr_uart_tests PRIVATE metasurface_avr GTest::gtest_main)
    target_compile_options(avr_uart_tests PRIVATE -Wall -Wextra -Wpedantic)
    gtest_discover_tests(avr_uart_tests)

    add_executable(controller_application_tests tests/controller_application_test.cpp)
    target_link_libraries(controller_application_tests PRIVATE metasurface_core GTest::gtest_main)
    target_compile_options(controller_application_tests PRIVATE -Wall -Wextra -Wpedantic)
    gtest_discover_tests(controller_application_tests)
endif()
