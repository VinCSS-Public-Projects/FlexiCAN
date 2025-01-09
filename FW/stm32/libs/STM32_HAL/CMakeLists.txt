project(STM32_HAL)


set(STM32G4_SOURCES
    config/stm32g4xx_hal_conf.h

    include/stm32g4xx/stm32g4xx_hal_def.h
    include/stm32g4xx/stm32g4xx_hal.h
    src/stm32g4xx/stm32g4xx_hal.c

    include/stm32g4xx/stm32g4xx_hal_fdcan.h
    src/stm32g4xx/stm32g4xx_hal_fdcan.c

    include/stm32g4xx/stm32g4xx_hal_cortex.h
    src/stm32g4xx/stm32g4xx_hal_cortex.c

    include/stm32g4xx/stm32g4xx_hal_flash_ex.h
    src/stm32g4xx/stm32g4xx_hal_flash_ex.c
    include/stm32g4xx/stm32g4xx_hal_flash.h
    src/stm32g4xx/stm32g4xx_hal_flash.c

    include/stm32g4xx/stm32g4xx_hal_gpio_ex.h
    include/stm32g4xx/stm32g4xx_hal_gpio.h
    src/stm32g4xx/stm32g4xx_hal_gpio.c

    include/stm32g4xx/stm32g4xx_hal_pcd_ex.h
    src/stm32g4xx/stm32g4xx_hal_pcd_ex.c
    include/stm32g4xx/stm32g4xx_hal_pcd.h
    src/stm32g4xx/stm32g4xx_hal_pcd.c

    include/stm32g4xx/stm32g4xx_hal_rcc.h
    src/stm32g4xx/stm32g4xx_hal_rcc.c
    include/stm32g4xx/stm32g4xx_hal_rcc_ex.h
    src/stm32g4xx/stm32g4xx_hal_rcc_ex.c

    include/stm32g4xx/stm32g4xx_hal_pwr.h
    src/stm32g4xx/stm32g4xx_hal_pwr.c
    include/stm32g4xx/stm32g4xx_hal_pwr_ex.h
    src/stm32g4xx/stm32g4xx_hal_pwr_ex.c

    include/stm32g4xx/stm32g4xx_hal_tim_ex.h
    src/stm32g4xx/stm32g4xx_hal_tim_ex.c
    include/stm32g4xx/stm32g4xx_hal_tim.h
    src/stm32g4xx/stm32g4xx_hal_tim.c

    include/stm32g4xx/stm32g4xx_ll_usb.h
    src/stm32g4xx/stm32g4xx_ll_usb.c

    src/cmsis/system_stm32g4xx.c
    include/stm32g4xx/Legacy/stm32_hal_legacy.h

    include/cmsis/cmsis_compiler.h
    include/cmsis/cmsis_device.h
    include/cmsis/cmsis_gcc.h
    include/cmsis/cmsis_version.h
    include/cmsis/core_cm4.h
    include/cmsis/device/stm32g4xx.h
    include/cmsis/device/stm32g431xx.h
    include/cmsis/device/system_stm32g4xx.h
    config/hal_include.h
    include/stm32g4xx/Legacy/stm32_hal_legacy.h
)

set(INCLUDE_DIRS
    include/
    include/cmsis
    include/cmsis/device
    config/
)

add_library(STM32_HAL_STM32G431x8 STATIC ${STM32G4_SOURCES})
    target_include_directories(STM32_HAL_STM32G431x8 PUBLIC ${INCLUDE_DIRS} include/stm32g4xx)
    target_compile_options(STM32_HAL_STM32G431x8 PRIVATE ${CPUFLAGS_G4} -Wno-unused-parameter)
    target_compile_definitions(STM32_HAL_STM32G431x8 PUBLIC STM32G431xx HAL_TARGET_PREFIX=stm32g431xx)
