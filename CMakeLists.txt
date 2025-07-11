# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.1)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.1.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(monitoramento_suinos C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Caminho para o FreeRTOS
set(FREERTOS_KERNEL_PATH ${CMAKE_CURRENT_SOURCE_DIR}/lib/FreeRTOS-Kernel)

# Importa o FreeRTOS
include(${FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2040/FreeRTOS_Kernel_import.cmake)

# Add executable. Default name is the project name, version 0.1

add_executable(monitoramento_suinos 
        main.c 
        src/aht10.c
        src/balanca.c
        src/rtc_ntp.c
        src/mqtt_client.c
        src/setup.c
        src/controle.c
        lib/mfrc522.c
        lib/pico_servo.c

        ${FREERTOS_KERNEL_PATH}/tasks.c
        ${FREERTOS_KERNEL_PATH}/timers.c
        ${FREERTOS_KERNEL_PATH}/queue.c
        ${FREERTOS_KERNEL_PATH}/event_groups.c
        ${FREERTOS_KERNEL_PATH}/list.c
        ${FREERTOS_KERNEL_PATH}/portable/MemMang/heap_4.c
        )

pico_set_program_name(monitoramento_suinos "monitoramento_suinos")
pico_set_program_version(monitoramento_suinos "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(monitoramento_suinos 0)
pico_enable_stdio_usb(monitoramento_suinos 1)

# Add the standard library to the build
target_link_libraries(monitoramento_suinos
        pico_stdlib
        hardware_i2c
        hardware_rtc
        hardware_spi
        hardware_timer
        hardware_clocks
        hardware_gpio
        hardware_irq
        hardware_pwm
        pico_cyw43_arch_lwip_threadsafe_background
        pico_lwip_mqtt
        pico_util
        FreeRTOS-Kernel
        )

# Add the standard include files to the build
target_include_directories(monitoramento_suinos PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/inc
        ${CMAKE_CURRENT_LIST_DIR}/src
        ${CMAKE_CURRENT_LIST_DIR}/../config
        ${CMAKE_CURRENT_LIST_DIR}/../tasks
        ${CMAKE_CURRENT_LIST_DIR}/../drivers
)

# Add any user requested libraries
target_link_libraries(monitoramento_suinos 
        )

pico_add_extra_outputs(monitoramento_suinos)

