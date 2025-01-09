project(STM32_USB_Device_Library)

set(SOURCES
    Core/Inc/usbd_def.h
    Core/Inc/usbd_ctlreq.h
    Core/Src/usbd_ctlreq.c
    Core/Inc/usbd_ioreq.h
    Core/Src/usbd_ioreq.c
    Core/Inc/usbd_core.h
    Core/Src/usbd_core.c
)

set(INCLUDE_DIRS
    Core/Inc/
    config/
    ../STM32_HAL/config/
)

add_library(STM32_USB_Device_Library_STM32G431x8 STATIC ${SOURCES})
    target_include_directories(STM32_USB_Device_Library_STM32G431x8 PUBLIC ${INCLUDE_DIRS})
    target_compile_options(STM32_USB_Device_Library_STM32G431x8 PRIVATE ${CPUFLAGS_G4} -Wno-unused-parameter -DSTM32G4)
    target_link_libraries(STM32_USB_Device_Library_STM32G431x8 PRIVATE STM32_HAL_STM32G431x8)