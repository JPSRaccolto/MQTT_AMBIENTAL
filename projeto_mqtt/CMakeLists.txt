# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

set(PICO_BOARD pico_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(projeto_mqtt C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(projeto_mqtt projeto_mqtt.c lib/ssd1306.c)

pico_set_program_name(projeto_mqtt "projeto_mqtt")
pico_set_program_version(projeto_mqtt "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(projeto_mqtt 1)
pico_enable_stdio_usb(projeto_mqtt 1)

# Add the standard library to the build
target_link_libraries(projeto_mqtt
    pico_stdlib
    hardware_adc
    pico_cyw43_arch_lwip_threadsafe_background
    pico_lwip_mqtt
    pico_mbedtls
    pico_lwip_mbedtls
    hardware_pwm
    hardware_i2c)

# Add the standard include files to the build
target_include_directories(projeto_mqtt PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/lib
)

# Add any user requested libraries
target_link_libraries(projeto_mqtt 
        
        )

pico_add_extra_outputs(projeto_mqtt)

