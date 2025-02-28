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

*/

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "board.h"
#include "can.h"
#include "can_common.h"
#include "config.h"
#include "device.h"
#include "dfu.h"
#include "gpio.h"
#include "gs_usb.h"
#include "hal_include.h"
#include "led.h"
#include "timer.h"
#include "usbd_conf.h"
#include "usbd_core.h"
#include "usbd_def.h"
#include "usbd_desc.h"
#include "usbd_gs_can.h"
#include "util.h"

static USBD_GS_CAN_HandleTypeDef hGS_CAN;
static USBD_HandleTypeDef hUSB = {0};

static void DisableUSBSOFInterrupt(USBD_HandleTypeDef *pdev)
{
#if defined(USB) || defined(USB_DRD_FS)
	// F0, G0 and G4 do not respect sof_enable field in their init structures
	PCD_HandleTypeDef *pcd = (PCD_HandleTypeDef*)pdev->pData;

#if defined(USB_DRD_FS)
	// G0
	USB_DRD_TypeDef *usb = pcd->Instance;
#else
	// F0 and G4
	USB_TypeDef *usb = pcd->Instance;
#endif

	usb->CNTR &= ~(USB_CNTR_SOFM | USB_CNTR_ESOFM);
#else
	(void)pdev;
#endif
}

int main(void)
{
	HAL_Init();
	device_sysclock_config();

	config.setup(&hGS_CAN);
	timer_init();

	INIT_LIST_HEAD(&hGS_CAN.list_frame_pool);
	INIT_LIST_HEAD(&hGS_CAN.list_to_host);

	for (unsigned i = 0; i < ARRAY_SIZE(hGS_CAN.msgbuf); i++) {
		list_add_tail(&hGS_CAN.msgbuf[i].list, &hGS_CAN.list_frame_pool);
	}

	for (unsigned int i = 0; i < ARRAY_SIZE(hGS_CAN.channels); i++) {
		const struct BoardChannelConfig *channel_config = &config.channels[i];
		const struct LEDConfig *led_config = channel_config->leds;
		can_data_t *channel = &hGS_CAN.channels[i];

		channel->nr = i;

		INIT_LIST_HEAD(&channel->list_from_host);

		led_init(&channel->leds,
				 led_config[LED_RX].port, led_config[LED_RX].pin, led_config[LED_RX].active_high, led_config[LED_RX].invert,
				 led_config[LED_TX].port, led_config[LED_TX].pin, led_config[LED_TX].active_high, led_config[LED_TX].invert);

		/* nice wake-up pattern */
		for (uint8_t j = 0; j < 200; j++) {
			HAL_GPIO_TogglePin(led_config[LED_RX].port, led_config[LED_RX].pin);
			HAL_Delay(1);
			HAL_GPIO_TogglePin(led_config[LED_TX].port, led_config[LED_TX].pin);
		}

		led_set_mode(&channel->leds, LED_MODE_OFF);

		can_init(channel, config.channels[i].interface);
		can_disable(channel);
	}

	USBD_Init(&hUSB, (USBD_DescriptorsTypeDef*)&FS_Desc, DEVICE_FS);
	USBD_RegisterClass(&hUSB, &USBD_GS_CAN);
	USBD_GS_CAN_Init(&hGS_CAN, &hUSB);
	USBD_Start(&hUSB);
	DisableUSBSOFInterrupt(&hUSB);

	while (1) {

		USBD_GS_CAN_SendReceiveFromHost(&hUSB);

		for (unsigned int i = 0; i < ARRAY_SIZE(hGS_CAN.channels); i++) {
			can_data_t *channel = &hGS_CAN.channels[i];

			CAN_SendFrame(&hGS_CAN, channel);
			CAN_ReceiveFrame(&hGS_CAN, channel);
			CAN_HandleError(&hGS_CAN, channel);

			led_update(&channel->leds);
		}

		if (USBD_GS_CAN_DfuDetachRequested(&hUSB)) {
			dfu_run_bootloader();
		}

		if (config.mainloop_callback) config.mainloop_callback();
	}
}
