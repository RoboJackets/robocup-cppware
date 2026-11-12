cmake_minimum_required(VERSION 3.22)
# Enable CMake support for ASM and C languages
enable_language(C ASM)

set(LM75_Include_Dirs
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/LM75/Inc
)

# Interface library for includes and symbols
add_library(lm75 OBJECT)
target_include_directories(lm75 PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/LM75/Inc
)
target_sources(lm75 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/LM75/Src/lm75.c
)

# Add the map file to the list of files to be removed with 'clean' target
set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES ADDITIONAL_CLEAN_FILES ${CMAKE_PROJECT_NAME}.map)

# Validate that STM32CubeMX code is compatible with C standard
if((CMAKE_C_STANDARD EQUAL 90) OR (CMAKE_C_STANDARD EQUAL 99))
    message(ERROR "Generated code requires C11 or higher")
endif()