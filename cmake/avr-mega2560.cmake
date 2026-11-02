set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR avr)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

find_program(CMAKE_CXX_COMPILER NAMES avr-g++ REQUIRED)
find_program(CMAKE_AR NAMES avr-ar REQUIRED)
find_program(CMAKE_RANLIB NAMES avr-ranlib REQUIRED)

set(CMAKE_CXX_FLAGS_INIT "-mmcu=atmega2560 -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti")
set(CMAKE_EXE_LINKER_FLAGS_INIT "-Wl,--gc-sections")
