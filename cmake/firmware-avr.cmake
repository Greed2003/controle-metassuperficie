find_program(AVR_OBJCOPY NAMES avr-objcopy REQUIRED)
find_program(AVR_OBJDUMP NAMES avr-objdump REQUIRED)

add_executable(metasurface_firmware firmware/main.cpp)
target_link_libraries(metasurface_firmware PRIVATE metasurface_avr)
target_compile_options(metasurface_firmware PRIVATE -Wall -Wextra -Wpedantic)
set_target_properties(metasurface_firmware PROPERTIES SUFFIX ".elf")

set(firmware_base "${CMAKE_CURRENT_BINARY_DIR}/metasurface_firmware")
target_link_options(metasurface_firmware PRIVATE "-Wl,-Map,${firmware_base}.map")
set_property(TARGET metasurface_firmware APPEND PROPERTY
    ADDITIONAL_CLEAN_FILES "${firmware_base}.map")

add_custom_target(firmware_artifacts ALL
    COMMAND "${CMAKE_COMMAND}" -E rm -f "${firmware_base}.hex" "${firmware_base}.size.txt"
    COMMAND "${CMAKE_COMMAND}"
        "-DELF=$<TARGET_FILE:metasurface_firmware>"
        "-DAVR_OBJDUMP=${AVR_OBJDUMP}"
        "-DREPORT=${firmware_base}.size.txt"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/ci/check-avr-firmware.cmake"
    COMMAND "${AVR_OBJCOPY}" -O ihex -j .text -j .data
        "$<TARGET_FILE:metasurface_firmware>" "${firmware_base}.hex"
    DEPENDS metasurface_firmware "${CMAKE_CURRENT_SOURCE_DIR}/ci/check-avr-firmware.cmake"
    BYPRODUCTS "${firmware_base}.hex" "${firmware_base}.size.txt"
    VERBATIM
)
