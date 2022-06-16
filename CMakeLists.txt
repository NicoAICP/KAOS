cmake_minimum_required(VERSION 3.12)
include(pico_sdk_import.cmake)

project(pico_examples C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.3.0")
    message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.3.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

set(PICO_EXAMPLES_PATH ${PROJECT_SOURCE_DIR})

# Initialize the SDK
pico_sdk_init()


add_executable(skylander_portal_emu
        main.c
        )

target_sources(skylander_portal_emu PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/main.c
        ${CMAKE_CURRENT_LIST_DIR}/usb_descriptors.c
        )
# pull in common dependencies
target_link_libraries(skylander_portal_emu PUBLIC pico_stdlib tinyusb_device tinyusb_board hardware_i2c hardware_gpio)

# create map/bin/hex/uf2 file etc.
pico_add_extra_outputs(skylander_portal_emu)


