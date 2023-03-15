cmake_minimum_required(VERSION 3.13)

project(SensorGauge C CXX)
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

include("PicoLed/PicoLed.cmake")

# rest of your project
add_executable(SensorGauge low_power.c i2c.c SensorGauge.cpp)

pico_set_program_name(SensorGauge "SensorGauge")
pico_set_program_version(SensorGauge "0.1")

pico_enable_stdio_usb(SensorGauge 0)
pico_enable_stdio_uart(SensorGauge 0)

# Add pico_stdlib library which aggregates commonly used features
target_link_libraries(SensorGauge pico_stdlib pico_runtime pico_i2c_slave hardware_sleep hardware_rtc PicoLed)

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(SensorGauge)
