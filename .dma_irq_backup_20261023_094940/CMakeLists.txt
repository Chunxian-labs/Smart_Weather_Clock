# CMake file for STM32F407VET6 with SPL
cmake_minimum_required(VERSION 3.15.3)

set(CMAKE_VERBOSE_MAKEFILE ON)

# Path to toolchain file (must be before project())
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/arm-none-eabi-gcc.cmake)

# Setup project, output and linker file
project(Smart_Weather_Clock)
set(EXECUTABLE ${PROJECT_NAME}.elf)
set(LINKER_FILE ${CMAKE_SOURCE_DIR}/STM32F407XX_FLASH.ld)

enable_language(C ASM)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

message(STATUS "CMAKE_TOOLCHAIN_FILE is: ${CMAKE_TOOLCHAIN_FILE}")

# Source files
set(SRC_FILES
        # Application
        app/main.c
        app/application/application.c
        app/ui/ui.c
        app/weather/weather.c
        app/wifi/wifi.c
        app/bsp_module/bsp.c

        # Board drivers (SPL-based)
        drivers/aht20/aht20.c
        drivers/esp32/esp32.c
        drivers/st7789/st7789.c

        # Board system peripherals(BSP)
        # bsp/cpu_tick/cpu_tick.c
        bsp/i2c/i2c.c
        bsp/rtc/rtc.c
        bsp/spi/spi.c
        bsp/uart/uart.c
        bsp/st7789_port/st7789_port.c

        # CMSIS device files
        Firmware/CMSIS/device/startup_stm32f407xx.s
        Firmware/CMSIS/device/stm32f4xx_it.c
        Firmware/CMSIS/device/system_stm32f4xx.c

        # SPL Standard Peripheral Driver
        Firmware/driver/src/misc.c
        Firmware/driver/src/stm32f4xx_gpio.c
        Firmware/driver/src/stm32f4xx_rcc.c
        Firmware/driver/src/stm32f4xx_usart.c
        Firmware/driver/src/stm32f4xx_exti.c
        Firmware/driver/src/stm32f4xx_syscfg.c
        Firmware/driver/src/stm32f4xx_flash.c
        Firmware/driver/src/stm32f4xx_tim.c
        Firmware/driver/src/stm32f4xx_pwr.c
        Firmware/driver/src/stm32f4xx_i2c.c
        Firmware/driver/src/stm32f4xx_spi.c
        Firmware/driver/src/stm32f4xx_rtc.c
        Firmware/driver/src/stm32f4xx_dma.c

        # Third_lib
        third_lib/fonts/font16_maple.c
        third_lib/fonts/font20_maple_bold.c
        third_lib/fonts/font24_maple_bold.c
        third_lib/fonts/font32_maple_bold.c
        third_lib/fonts/font54_maple_bold.c
        third_lib/fonts/font76_maple_extrabold.c

        third_lib/images/welcome_page_pic.c
        third_lib/images/error_page_pic.c
        third_lib/images/wifi_page_pic.c
        third_lib/images/cloudy.c
        third_lib/images/snowy.c
        third_lib/images/rainy.c
        third_lib/images/overcast.c
        third_lib/images/rainstorm.c
        third_lib/images/windy.c
        third_lib/images/thermometer.c
        third_lib/images/moon.c
        third_lib/images/sunny.c
        third_lib/images/wifi_pic.c
        third_lib/freertos/portable/port.c
        third_lib/freertos/portable/heap_4.c
        third_lib/freertos/source/croutine.c
        third_lib/freertos/source/event_groups.c
        third_lib/freertos/source/list.c
        third_lib/freertos/source/queue.c
        third_lib/freertos/source/stream_buffer.c
        third_lib/freertos/source/tasks.c
        third_lib/freertos/source/timers.c

        # Utilities
        utilities/ring_buffer.c

    )

# Build the executable
add_executable(${EXECUTABLE} ${SRC_FILES})

# Compiler defines
target_compile_definitions(${EXECUTABLE} PRIVATE
        -DSTM32F40_41xxx
        -DUSE_STDPERIPH_DRIVER
        -DHSE_VALUE=8000000
        )

# Include directories
target_include_directories(${EXECUTABLE} PRIVATE
        # CMSIS core
        Firmware/CMSIS/core
        # CMSIS device (stm32f4xx.h, system_stm32f4xx.h)
        Firmware/CMSIS/device
        # SPL driver headers
        Firmware/driver/inc
        # Application layer
        app/application
        app/ui
        app/weather
        app/wifi
        app/bsp_module
        # Board layer
        drivers/aht20
        drivers/esp32
        drivers/st7789
        # Board system peripherals(BSP)
        # bsp/cpu_tick
        bsp/i2c
        bsp/rtc
        bsp/spi
        bsp/uart
        bsp/st7789_port
        # Third_lib
        third_lib/fonts
        third_lib/images
        third_lib/freertos/include
        third_lib/freertos/portable
        # Utilities
        utilities
        )

# Compiler options
target_compile_options(${EXECUTABLE} PRIVATE
        -mcpu=cortex-m4
        -mfpu=fpv4-sp-d16 
        -mfloat-abi=hard
        -mthumb

        -fdata-sections
        -ffunction-sections

        -Wall
        -O0
        -g3
        )

# Linker options
target_link_options(${EXECUTABLE} PRIVATE
        -T${LINKER_FILE}
        -mcpu=cortex-m4
        -mfpu=fpv4-sp-d16 
        -mfloat-abi=hard
        -mthumb
        -specs=nano.specs
        -specs=nosys.specs
        -lc
        -lm
        -Wl,-Map=${PROJECT_NAME}.map,--cref
        -Wl,--gc-sections
        -Xlinker -print-memory-usage -Xlinker
        -u _printf_float
        )

# Post-build: print size
add_custom_command(TARGET ${EXECUTABLE}
        POST_BUILD
        COMMAND ${CMAKE_SIZE_UTIL} ${EXECUTABLE})

# Post-build: create hex, bin, srec
add_custom_command(TARGET ${EXECUTABLE}
        POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O srec --srec-len=64 ${EXECUTABLE} ${PROJECT_NAME}.s19
        COMMAND ${CMAKE_OBJCOPY} -O ihex ${EXECUTABLE} ${PROJECT_NAME}.hex
        COMMAND ${CMAKE_OBJCOPY} -O binary ${EXECUTABLE} ${PROJECT_NAME}.bin)
