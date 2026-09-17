# Source lists and flags imported from the existing Eclipse project.
cmake_minimum_required(VERSION 3.24)
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/../cmake/arm-none-eabi.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/cmake/target.cmake")
project(CubeIDE_H745II_UWB3_CM4 LANGUAGES C ASM)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_executable(${PROJECT_NAME})
set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".elf")
target_include_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/decadriver"
    "${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/platform"
    "${CMAKE_CURRENT_SOURCE_DIR}/Core/Inc"
    "${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/STM32H7xx_HAL_Driver/Inc"
    "${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy"
    "${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/CMSIS/Device/ST/STM32H7xx/Include"
    "${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/CMSIS/Include"
    "${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/flash"
)
target_sources(${PROJECT_NAME} PRIVATE
    "../Common/Src/system_stm32h7xx_dualcore_boot_cm4_cm7.c"
    "Core/Src/algorithm.c"
    "Core/Src/decadriver/deca_device.c"
    "Core/Src/dma.c"
    "Core/Src/flash/flash_if.c"
    "Core/Src/gpio.c"
    "Core/Src/instance.c"
    "Core/Src/main.c"
    "Core/Src/platform/deca_mutex.c"
    "Core/Src/platform/deca_sleep.c"
    "Core/Src/platform/deca_spi.c"
    "Core/Src/platform/port.c"
    "Core/Src/spi.c"
    "Core/Src/stm32h7xx_hal_msp.c"
    "Core/Src/stm32h7xx_it.c"
    "Core/Src/syscalls.c"
    "Core/Src/sysmem.c"
    "Core/Src/usart.c"
    "Core/Startup/startup_stm32h745iitx.s"
    "../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal.c"
    "../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_cortex.c"
    "../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_dma.c"
    "../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_dma_ex.c"
    "../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_exti.c"
    "../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_flash.c"
    "../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_flash_ex.c"
    "../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_gpio.c"
    "../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_hsem.c"
    "../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_i2c.c"
    "../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_i2c_ex.c"
    "../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_mdma.c"
    "../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_pwr.c"
    "../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_pwr_ex.c"
    "../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_rcc.c"
    "../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_rcc_ex.c"
    "../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_spi.c"
    "../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_spi_ex.c"
    "../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_tim.c"
    "../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_tim_ex.c"
    "../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_uart.c"
    "../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_uart_ex.c"
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  target_compile_options(${PROJECT_NAME} PRIVATE "$<$<COMPILE_LANGUAGE:C>:-Wno-error=implicit-function-declaration>")
  target_compile_options(${PROJECT_NAME} PRIVATE "$<$<COMPILE_LANGUAGE:C>:-fcommon>")
  target_compile_options(${PROJECT_NAME} PRIVATE "$<$<COMPILE_LANGUAGE:C>:-mfpu=fpv4-sp-d16>")
  target_compile_options(${PROJECT_NAME} PRIVATE "$<$<COMPILE_LANGUAGE:C>:-mfloat-abi=hard>")
  target_compile_options(${PROJECT_NAME} PRIVATE "$<$<COMPILE_LANGUAGE:C>:-mthumb>")
  target_compile_options(${PROJECT_NAME} PRIVATE "$<$<COMPILE_LANGUAGE:C>:--specs=nano.specs>")
  target_compile_options(${PROJECT_NAME} PRIVATE "$<$<COMPILE_LANGUAGE:C>:-u>")
  target_compile_options(${PROJECT_NAME} PRIVATE "$<$<COMPILE_LANGUAGE:C>:_printf_float>")
  target_compile_options(${PROJECT_NAME} PRIVATE "$<$<COMPILE_LANGUAGE:C>:-mcpu=cortex-m4>")
  target_compile_options(${PROJECT_NAME} PRIVATE "$<$<COMPILE_LANGUAGE:C>:-std=gnu11>")
  target_compile_options(${PROJECT_NAME} PRIVATE "$<$<COMPILE_LANGUAGE:C>:-g3>")
  target_compile_options(${PROJECT_NAME} PRIVATE "$<$<COMPILE_LANGUAGE:C>:-DDEBUG>")
  target_compile_options(${PROJECT_NAME} PRIVATE "$<$<COMPILE_LANGUAGE:C>:-DCORE_CM4>")
  target_compile_options(${PROJECT_NAME} PRIVATE "$<$<COMPILE_LANGUAGE:C>:-DUSE_HAL_DRIVER>")
  target_compile_options(${PROJECT_NAME} PRIVATE "$<$<COMPILE_LANGUAGE:C>:-DSTM32H745xx>")
  target_compile_options(${PROJECT_NAME} PRIVATE "$<$<COMPILE_LANGUAGE:C>:-O0>")
  target_compile_options(${PROJECT_NAME} PRIVATE "$<$<COMPILE_LANGUAGE:C>:-ffunction-sections>")
  target_compile_options(${PROJECT_NAME} PRIVATE "$<$<COMPILE_LANGUAGE:C>:-fdata-sections>")
  target_compile_options(${PROJECT_NAME} PRIVATE "$<$<COMPILE_LANGUAGE:C>:-Wall>")
  target_compile_options(${PROJECT_NAME} PRIVATE "$<$<COMPILE_LANGUAGE:C>:-fstack-usage>")
  target_compile_options(${PROJECT_NAME} PRIVATE "$<$<COMPILE_LANGUAGE:C>:-fcyclomatic-complexity>")
  target_compile_options(${PROJECT_NAME} PRIVATE "$<$<COMPILE_LANGUAGE:ASM>:-mfpu=fpv4-sp-d16>")
  target_compile_options(${PROJECT_NAME} PRIVATE "$<$<COMPILE_LANGUAGE:ASM>:-mfloat-abi=hard>")
  target_compile_options(${PROJECT_NAME} PRIVATE "$<$<COMPILE_LANGUAGE:ASM>:-mthumb>")
  target_compile_options(${PROJECT_NAME} PRIVATE "$<$<COMPILE_LANGUAGE:ASM>:--specs=nano.specs>")
  target_compile_options(${PROJECT_NAME} PRIVATE "$<$<COMPILE_LANGUAGE:ASM>:-u>")
  target_compile_options(${PROJECT_NAME} PRIVATE "$<$<COMPILE_LANGUAGE:ASM>:_printf_float>")
  target_compile_options(${PROJECT_NAME} PRIVATE "$<$<COMPILE_LANGUAGE:ASM>:-mcpu=cortex-m4>")
  target_compile_options(${PROJECT_NAME} PRIVATE "$<$<COMPILE_LANGUAGE:ASM>:-g3>")
  target_compile_options(${PROJECT_NAME} PRIVATE "$<$<COMPILE_LANGUAGE:ASM>:-DDEBUG>")
  target_compile_options(${PROJECT_NAME} PRIVATE "$<$<COMPILE_LANGUAGE:ASM>:-x>")
  target_compile_options(${PROJECT_NAME} PRIVATE "$<$<COMPILE_LANGUAGE:ASM>:assembler-with-cpp>")
  target_link_options(${PROJECT_NAME} PRIVATE
    "-mfpu=fpv4-sp-d16"
    "-mfloat-abi=hard"
    "-mthumb"
    "--specs=nano.specs"
    "-u"
    "_printf_float"
    "-mcpu=cortex-m4"
    "-T${CMAKE_CURRENT_SOURCE_DIR}/STM32H745IITX_FLASH.ld"
    "--specs=nosys.specs"
    "-Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.map"
    "-Wl,--gc-sections"
    "-static"
  )
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
  target_compile_options(${PROJECT_NAME} PRIVATE "$<$<COMPILE_LANGUAGE:C>:-Wno-error=implicit-function-declaration>")
  target_compile_options(${PROJECT_NAME} PRIVATE "$<$<COMPILE_LANGUAGE:C>:-fcommon>")
  target_compile_options(${PROJECT_NAME} PRIVATE "$<$<COMPILE_LANGUAGE:C>:-mfpu=fpv4-sp-d16>")
  target_compile_options(${PROJECT_NAME} PRIVATE "$<$<COMPILE_LANGUAGE:C>:-mfloat-abi=hard>")
  target_compile_options(${PROJECT_NAME} PRIVATE "$<$<COMPILE_LANGUAGE:C>:-mthumb>")
  target_compile_options(${PROJECT_NAME} PRIVATE "$<$<COMPILE_LANGUAGE:C>:--specs=nano.specs>")
  target_compile_options(${PROJECT_NAME} PRIVATE "$<$<COMPILE_LANGUAGE:C>:-mcpu=cortex-m4>")
  target_compile_options(${PROJECT_NAME} PRIVATE "$<$<COMPILE_LANGUAGE:C>:-std=gnu11>")
  target_compile_options(${PROJECT_NAME} PRIVATE "$<$<COMPILE_LANGUAGE:C>:-DCORE_CM4>")
  target_compile_options(${PROJECT_NAME} PRIVATE "$<$<COMPILE_LANGUAGE:C>:-DUSE_HAL_DRIVER>")
  target_compile_options(${PROJECT_NAME} PRIVATE "$<$<COMPILE_LANGUAGE:C>:-DSTM32H745xx>")
  target_compile_options(${PROJECT_NAME} PRIVATE "$<$<COMPILE_LANGUAGE:C>:-Os>")
  target_compile_options(${PROJECT_NAME} PRIVATE "$<$<COMPILE_LANGUAGE:C>:-ffunction-sections>")
  target_compile_options(${PROJECT_NAME} PRIVATE "$<$<COMPILE_LANGUAGE:C>:-fdata-sections>")
  target_compile_options(${PROJECT_NAME} PRIVATE "$<$<COMPILE_LANGUAGE:C>:-Wall>")
  target_compile_options(${PROJECT_NAME} PRIVATE "$<$<COMPILE_LANGUAGE:C>:-fstack-usage>")
  target_compile_options(${PROJECT_NAME} PRIVATE "$<$<COMPILE_LANGUAGE:C>:-fcyclomatic-complexity>")
  target_compile_options(${PROJECT_NAME} PRIVATE "$<$<COMPILE_LANGUAGE:ASM>:-mfpu=fpv4-sp-d16>")
  target_compile_options(${PROJECT_NAME} PRIVATE "$<$<COMPILE_LANGUAGE:ASM>:-mfloat-abi=hard>")
  target_compile_options(${PROJECT_NAME} PRIVATE "$<$<COMPILE_LANGUAGE:ASM>:-mthumb>")
  target_compile_options(${PROJECT_NAME} PRIVATE "$<$<COMPILE_LANGUAGE:ASM>:--specs=nano.specs>")
  target_compile_options(${PROJECT_NAME} PRIVATE "$<$<COMPILE_LANGUAGE:ASM>:-mcpu=cortex-m4>")
  target_compile_options(${PROJECT_NAME} PRIVATE "$<$<COMPILE_LANGUAGE:ASM>:-x>")
  target_compile_options(${PROJECT_NAME} PRIVATE "$<$<COMPILE_LANGUAGE:ASM>:assembler-with-cpp>")
  target_link_options(${PROJECT_NAME} PRIVATE
    "-mfpu=fpv4-sp-d16"
    "-mfloat-abi=hard"
    "-mthumb"
    "--specs=nano.specs"
    "-mcpu=cortex-m4"
    "-T${CMAKE_CURRENT_SOURCE_DIR}/STM32H745IITX_FLASH.ld"
    "--specs=nosys.specs"
    "-Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.map"
    "-Wl,--gc-sections"
    "-static"
  )
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE "-Wl,--start-group" c m "-Wl,--end-group")
set_property(TARGET ${PROJECT_NAME} APPEND PROPERTY LINK_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/STM32H745IITX_FLASH.ld")
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND "${CMAKE_SIZE}" "$<TARGET_FILE:${PROJECT_NAME}>"
    COMMAND "${CMAKE_OBJCOPY}" -O ihex "$<TARGET_FILE:${PROJECT_NAME}>" "${PROJECT_NAME}.hex"
    COMMAND "${CMAKE_OBJCOPY}" -O binary "$<TARGET_FILE:${PROJECT_NAME}>" "${PROJECT_NAME}.bin"
    VERBATIM)
