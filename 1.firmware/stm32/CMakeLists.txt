cmake_minimum_required(VERSION 3.13)
project(candleLightFirmware C ASM)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(
	--specs=nano.specs
	--specs=nosys.specs
	-O2
	-Wall
	-Werror
	-Wextra
	-Wstrict-prototypes
	-fdata-sections
	-ffat-lto-objects
	-ffreestanding
	-ffunction-sections
	-flto
	-fmessage-length=0
	-fsigned-char
	-g3
	-mthumb
	-std=gnu11
)

#need these later, per-platform
set(CPUFLAGS_G4 -mcpu=cortex-m4)

add_link_options(
	--specs=nano.specs
	--specs=nosys.specs
	-Wall
	-Wextra
	-g3
	-mthumb
	LINKER:--gc-sections
	LINKER:--print-memory-usage
)

add_subdirectory(libs/STM32_HAL)
add_subdirectory(libs/STM32_USB_Device_Library)
set( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake )


# Add a custom target that produces version.h, plus
# a dummy output that's not actually produced, in order
# to force version.hmake to always be re-run before the build


add_custom_target(version_h
	BYPRODUCTS "${CMAKE_CURRENT_BINARY_DIR}/version.h"
	COMMAND ${CMAKE_COMMAND}
	-D SRCDIR="${CMAKE_CURRENT_SOURCE_DIR}"
	-P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/gitversion.cmake
)

set(
	SOURCE_FILES
	include/config.h

	include/gs_usb.h
	include/usbd_desc.h src/usbd_desc.c
	include/usbd_gs_can.h src/usbd_gs_can.c
	src/usbd_conf.c

	include/board.h
	include/can.h
	include/can_common.h src/can_common.c
	include/device.h
	include/dfu.h src/dfu.c
	include/gpio.h src/gpio.c
	include/led.h src/led.c
	include/timer.h src/timer.c
	include/util.h src/util.c

	src/startup.c
	src/main.c
	src/interrupts.c

	${CMAKE_CURRENT_BINARY_DIR}/version.h
)


####### some helpers to generate targets
string(TIMESTAMP BUILD_DATE "%Y%m%d_%H%M%S")
## objcopy to produce .bin file
function(make_bin_file target)
	add_custom_command(
		TARGET ${target} POST_BUILD
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
		BYPRODUCTS ${target}.bin
		COMMAND ${CMAKE_OBJCOPY} -O binary ${target} ${target}_${BUILD_DATE}.bin
	)
endfunction()

## objcopy to produce .hex file
function(make_hex_file target)
	add_custom_command(
		TARGET ${target} POST_BUILD
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
		BYPRODUCTS ${target}.hex
		COMMAND ${CMAKE_OBJCOPY} -O ihex ${target} ${target}_${BUILD_DATE}.hex
	)
endfunction()


## report size
function(show_object_size target)
	string(REPLACE "objcopy" "size" CMAKE_OBJSIZE "${CMAKE_OBJCOPY}")
	add_custom_command(
		TARGET ${target} POST_BUILD
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
		COMMAND ${CMAKE_OBJSIZE} ${target}
	)
endfunction()

find_package(DFUSuffix)
## run dfu-suffix to append DFU stuff and signature; generate relevant flash-* target
# TODO ? : run with execute_proces(... OUTPUT_QUIET ) instead of '... 1>/dev/null'

function(dfu_flash target)
	if (DFU_SUFFIX_EXECUTABLE)
		add_custom_command( TARGET ${target}
			BYPRODUCTS ${target}.dfu
			COMMAND ${CMAKE_OBJCOPY} -O binary ${target} ${target}.dfu
			COMMAND ${DFU_SUFFIX_EXECUTABLE} --add ${target}.dfu --vid 1d50 --pid 606f 1>/dev/null
			COMMENT "create and sign dfu bin file: ${TGTNAME}_fw"
		)

		add_custom_target( flash-${target}
			dfu-util -a 0 -R -s 0x08000000 -D ${target}.dfu
		)
	else()
		add_custom_target( flash-${target}
			dfu-util -d 1d50:606f -a 0 -R -s 0x08000000 -D ${target}.bin
		)
	endif()
endfunction()



######### ldscripts are mostly identical.
# There's many ways of handling this :
# - external script (e.g. python), see e.g. libopencm3
# - preprocess .ldscript with gcc (some caveats, since .ld syntax != C)
# - concatenate files with file(APPEND...) : I wasn't able to make this work
# - configure_file().
#
# With configure_file(), the ldscripts are generated
# at configure time.

function(populate_ldscript)
    set(prefix LDV)
    set(options)
    set(oneValueArgs
		CPU_FAMILY
		FLASH_START
		FLASH_SIZE
		RAM_START
		RAM_SIZE
		STACK_SIZE
		HEAP_SIZE)
	set(multiValueArgs)
	cmake_parse_arguments("${prefix}" "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

	# now we should have function-scope vars like LDV_FLASH_SIZE etc.
	# Produce a new file in the current builddir

	configure_file(ldscripts/ldscript_base.inc ${${prefix}_CPU_FAMILY}_processed.ld)
endfunction()


##################
# generate ldscripts for all supported targets.
# If this gets out of hand, an external solution may be better
# (e.g. like libopencm3 does with a python script)

populate_ldscript(CPU_FAMILY STM32G431X8
	FLASH_START 0x08000000
	FLASH_SIZE 64k
	RAM_START 0x20000000
	RAM_SIZE 32k
	STACK_SIZE 2k
	HEAP_SIZE 1k
)

######### commands for adding each target have a lot in common: make helper func.
# Split into two categories, F042-based and F072-based.

function(add_target_common TGTNAME CPU_FAMILY)
	add_executable(${TGTNAME}_fw ${SOURCE_FILES})
	add_dependencies(${TGTNAME}_fw version_h)
	target_include_directories(${TGTNAME}_fw PRIVATE include/ ${CMAKE_CURRENT_BINARY_DIR})
	target_link_options(${TGTNAME}_fw PRIVATE -T ${CPU_FAMILY}_processed.ld)
	make_bin_file(${TGTNAME}_fw)
	make_hex_file(${TGTNAME}_fw)  # Add this line to generate the .hex file
	dfu_flash(${TGTNAME}_fw)
	show_object_size(${TGTNAME}_fw)
endfunction()

function(add_g431_target TGTNAME)
	add_target_common(${TGTNAME} STM32G431X8)
	target_compile_definitions(${TGTNAME}_fw PRIVATE BOARD_${TGTNAME} STM32G4)
	target_compile_options(${TGTNAME}_fw BEFORE PRIVATE ${CPUFLAGS_G4})
	target_sources(${TGTNAME}_fw PRIVATE "src/boards/g431-${TGTNAME}.c")
	target_sources(${TGTNAME}_fw PRIVATE "src/can/m_can.c")
	target_sources(${TGTNAME}_fw PRIVATE "src/device/device_g4.c")
	target_link_options(${TGTNAME}_fw BEFORE PRIVATE ${CPUFLAGS_G4})
	target_link_libraries(${TGTNAME}_fw PRIVATE STM32_HAL_STM32G431x8 STM32_USB_Device_Library_STM32G431x8)
endfunction()

########## generate list of targets.
# the "_fw" part is appended automatically
set(TGTG431_LIST "Obit_FDCAN")



foreach (TGTNAME IN LISTS TGTG431_LIST)
	option(BUILD_${TGTNAME} "Build firmware for \"${TGTNAME}\" (default=yes)" ON)
	if (BUILD_${TGTNAME})
		add_g431_target(${TGTNAME})
	endif()
endforeach()

message("*******************")
message("You may now:\n\t-compile all targets ('make')\n\t-compile a single target (e.g. 'make cantact_fw'")
message("\t-flash a device (e.g. 'make flash-cantact_fw'")
