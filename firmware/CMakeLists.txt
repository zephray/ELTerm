# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# initalize pico_sdk from installed location
# (note this can come from environment, CMake cache etc)
set(PICO_SDK_PATH "/home/pi/Documents/pico-sdk")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(el-demo C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

add_executable(el-demo)

pico_generate_pio_header(el-demo ${CMAKE_CURRENT_LIST_DIR}/eldata.pio)

target_sources(el-demo PRIVATE
        main.c
        el.c
        graphics.c
        terminal.c
        serial.c
        )

pico_set_program_name(el-demo "el-demo")
pico_set_program_version(el-demo "0.1")

pico_enable_stdio_uart(el-demo 0)
pico_enable_stdio_usb(el-demo 0)

# Add the standard library to the build
target_link_libraries(el-demo pico_stdlib hardware_dma)

# Add any user requested libraries
target_link_libraries(el-demo
        hardware_pio
        )

pico_add_extra_outputs(el-demo)

