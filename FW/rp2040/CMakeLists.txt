cmake_minimum_required(VERSION 3.28)

set(PICO_SDK_PATH ${CMAKE_CURRENT_LIST_DIR}/external_libs/pico-sdk)
include(${CMAKE_CURRENT_LIST_DIR}/external_libs/pico-sdk/external/pico_sdk_import.cmake)

project(FlexiCAN_Controller C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1
set (INCLUDE_DIR ${CMAKE_CURRENT_LIST_DIR}/include)
set (SRC_DIR ${CMAKE_CURRENT_LIST_DIR}/src)
set (3RD_DIR ${CMAKE_CURRENT_LIST_DIR}/external_libs)

add_executable(FlexiCAN_Controller 
    ${SRC_DIR}/main.c
    ${SRC_DIR}/storage.c
    ${SRC_DIR}/menu.c
    ${SRC_DIR}/utils.c
    ${SRC_DIR}/shell.c
    ${SRC_DIR}/ch446q.c
    ${SRC_DIR}/ssd1306.c
    ${3RD_DIR}/CherrySH/cherryrl/chry_readline.c
    ${3RD_DIR}/CherrySH/chry_shell.c
    ${3RD_DIR}/CherrySH/builtin/help.c
    ${3RD_DIR}/CherrySH/builtin/shsize.c
    ${3RD_DIR}/CherrySH/builtin/login.c
    ${3RD_DIR}/CherryRB/chry_ringbuffer.c

    ${3RD_DIR}/CherryUSB/port/rp2040/usb_dc_rp2040.c
    ${3RD_DIR}/CherryUSB/class/audio/usbd_audio.c
    ${3RD_DIR}/CherryUSB/class/cdc/usbd_cdc.c
    ${3RD_DIR}/CherryUSB/class/dfu/usbd_dfu.c
    ${3RD_DIR}/CherryUSB/class/hid/usbd_hid.c
    ${3RD_DIR}/CherryUSB/class/msc/usbd_msc.c
    ${3RD_DIR}/CherryUSB/class/printer/usbd_printer.c
    ${3RD_DIR}/CherryUSB/class/video/usbd_video.c
    ${3RD_DIR}/CherryUSB/core/usbd_core.c
)

pico_set_program_name(FlexiCAN_Controller "FlexiCAN_Controller")
pico_set_program_version(FlexiCAN_Controller "0.1")

pico_set_linker_script(FlexiCAN_Controller  ${CMAKE_CURRENT_LIST_DIR}/memmap_custom.ld)

pico_enable_stdio_uart(FlexiCAN_Controller 0)
pico_enable_stdio_usb(FlexiCAN_Controller 1)

target_include_directories(FlexiCAN_Controller PRIVATE
  ${INCLUDE_DIR}
  ${3RD_DIR}/CherrySH/
  ${3RD_DIR}/CherryRB/
  ${3RD_DIR}/CherryUSB/class/audio
  ${3RD_DIR}/CherryUSB/class/cdc
  ${3RD_DIR}/CherryUSB/class/dfu
  ${3RD_DIR}/CherryUSB/class/hid
  ${3RD_DIR}/CherryUSB/class/hub
  ${3RD_DIR}/CherryUSB/class/midi
  ${3RD_DIR}/CherryUSB/class/msc
  ${3RD_DIR}/CherryUSB/class/mtp
  ${3RD_DIR}/CherryUSB/class/printer
  ${3RD_DIR}/CherryUSB/class/vendor
  ${3RD_DIR}/CherryUSB/class/video
  ${3RD_DIR}/CherryUSB/class/wireless
  ${3RD_DIR}/CherryUSB/common
  ${3RD_DIR}/CherryUSB/core
  ${3RD_DIR}/CherryUSB/port/rp2040
)

target_link_libraries(FlexiCAN_Controller 
        pico_stdlib
        hardware_i2c
        hardware_adc
        hardware_pio
        hardware_resets 
        hardware_irq
        hardware_pwm
        hardware_flash
        pico_multicore
        )

#uncomment this line to build uf2 file
pico_add_extra_outputs(FlexiCAN_Controller)
