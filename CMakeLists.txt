cmake_minimum_required(VERSION 3.13)

# some paths are hardcoded here, you may want
# to update cross compiler paths and pico sdk paths
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(CMAKE_FIND_ROOT_PATH /opt/gcc-arm-none-eabi)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

set(PICO_SDK_PATH "/home/matteo/devel/pico/pico-sdk")
set(PICO_EXTRAS_PATH "/home/matteo/devel/pico/pico-extras")

set(PICO_BOARD waveshare_rp2040_zero)

# initialize the SDK based on PICO_SDK_PATH
# note: this must happen before project()
include(pico_sdk_import.cmake)

# We also need PICO EXTRAS (for hardware/rosc)
include(pico_extras_import.cmake)

set(PICO_CXX_ENABLE_EXCEPTIONS 1)
set(PICO_CXX_ENABLE_RTTI 1)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

project(DavisWindRainGauge C CXX ASM)

include("PicoLed/PicoLed.cmake")

# rest of your project
add_executable(DavisWindRainGauge wind.c utils.c low_power.c i2c.c DavisWindRainGauge.cpp)

pico_set_program_name(DavisWindRainGauge "DavisWindRainGauge")
pico_set_program_version(DavisWindRainGauge "0.1")

pico_enable_stdio_usb(DavisWindRainGauge 0)
pico_enable_stdio_uart(DavisWindRainGauge 0)

# Add pico_stdlib library which aggregates commonly used features
target_link_libraries(DavisWindRainGauge pico_stdlib pico_runtime pico_i2c_slave hardware_rosc hardware_rtc PicoLed)

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(DavisWindRainGauge)