/*

The MIT License (MIT)

Copyright (c) 2016 Hubert Denkmair

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

*
*
* Some board-specific defines here, such as :
* - USB strings
* - LED pin assignments and polarity
* - other special pins to control CAN transceivers.
*
* CAN_S_PIN: Some CAN transceivers (e.g. TJA1050) have a "Silent mode in which the transmitter is disabled";
* enabled with this 'S' pin. If undefined, the corresponding code will be disabled.
*
* TERM_Pin: Add support for an externally controlled terminating resistor
*
*/

#pragma once

#include "version.h"

extern char obit_product_string[64];
extern char obit_interface_string[64];

#define CAN_QUEUE_SIZE				 (256 * NUM_CAN_CHANNEL)

#define USBD_VID					 0x1d50
#define USBD_PID_FS					 0x606f
#define USBD_LANGID_STRING			 1033
#define USBD_CONFIGURATION_STRING_FS (uint8_t*) GIT_HASH
#define USBD_INTERFACE_STRING_FS	 (uint8_t*) "gs_usb interface"


#if defined(BOARD_Obit_FDCAN)
	#define USBD_PRODUCT_STRING_FS	 (uint8_t*) obit_product_string
	#define USBD_MANUFACTURER_STRING (uint8_t*) "VinCSS_FDCAN"
	#define DFU_INTERFACE_STRING_FS	 (uint8_t*) obit_interface_string

	#define TIM2_CLOCK_SPEED		 144000000

	#define CAN_CLOCK_SPEED			 144000000
	#define NUM_CAN_CHANNEL			 1
	#define CONFIG_CANFD			 1

#else
	#error please define BOARD
#endif
