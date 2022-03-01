# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# initalize pico_sdk from installed location
# (note this can come from environment, CMake cache etc)
set(PICO_SDK_PATH "/home/wenting/pico/pico-sdk")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(elterm C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

add_executable(elterm)

pico_generate_pio_header(elterm ${CMAKE_CURRENT_LIST_DIR}/eldata.pio)

target_sources(elterm PRIVATE
        main.c
        el.c
        graphics.c
        terminal.c
        termcore.c
        serial.c
        usbhid.c
        )

pico_set_program_name(elterm "elterm")
pico_set_program_version(elterm "0.1")

pico_enable_stdio_uart(elterm 0)
pico_enable_stdio_usb(elterm 0)

target_include_directories(elterm PRIVATE ${CMAKE_CURRENT_LIST_DIR})

# Add the standard library to the build
target_link_libraries(elterm pico_stdlib hardware_dma tinyusb_host tinyusb_board)

# Add any user requested libraries
target_link_libraries(elterm
        hardware_pio
        )

pico_add_extra_outputs(elterm)

