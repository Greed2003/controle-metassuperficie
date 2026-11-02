if(NOT DEFINED ARCHIVE OR NOT EXISTS "${ARCHIVE}")
    message(FATAL_ERROR "Informe uma biblioteca AVR existente com -DARCHIVE=...")
endif()

find_program(AVR_OBJDUMP NAMES avr-objdump REQUIRED)

execute_process(
    COMMAND "${AVR_OBJDUMP}" -t -C "${ARCHIVE}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE symbols
    ERROR_VARIABLE error
)

if(NOT "${result}" STREQUAL "0")
    message(FATAL_ERROR "Falha ao inspecionar a biblioteca AVR: ${error}")
endif()

string(REGEX MATCH
    "[^\r\n]*metasurface::\\(anonymous namespace\\)::kPatterns[^\r\n]*"
    catalog_symbol "${symbols}"
)

if(NOT catalog_symbol MATCHES "[ \t]O[ \t]+\\.progmem\\.data(\\.[^ \t]+)?[ \t]+0*600[ \t]+")
    message(FATAL_ERROR "O catalogo deve ocupar 1536 bytes em PROGMEM. Simbolo: ${catalog_symbol}")
endif()

message(STATUS "Catalogo AVR verificado: 1536 bytes em PROGMEM.")
