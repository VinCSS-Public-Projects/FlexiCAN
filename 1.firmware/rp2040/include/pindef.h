#ifndef __PIN_DEF__
#define __PIN_DEF__

#ifdef __cplusplus
extern "C"
{
#endif

// hold pin definition of Flexican device
// GPIOs to control dfu mode of STM32
#define GPIO_RP_STM_RST 1
#define GPIO_RP_STM_BOOT0 2
#define GPIO_BUTTON_K1 2

// GPIOs to control OLED screen
#define GPIO_I2C_PORT i2c1
#define GPIO_I2C_SDA 6
#define GPIO_I2C_SCL 7
#define OLED_WIDTH 128
#define OLED_HEIGHT 32
#define I2C_ADDR 0x3c

// GPIOs to control led TX/RX
#define GPIO_CAN_LED_TX 8
#define GPIO_CAN_LED_RX 9

// GPIOs to control CH466
#define GPIO_SW_PS 11
#define GPIO_SW_RST 21
#define GPIO_SW_DAT 20
#define GPIO_SW_CLK 19
#define GPIO_SW_STB 18

// GPIO to control CAN source either from RP or STM32
#define GPIO_CAN_SOURCE 23
// GPIO for control 120Ohm for CAN
#define GPIO_CAN_RES 22

#define GPIO_CAN_HACK_TX 29
#define GPIO_CAN_HACK_RX 28

// protected volt for C446
#define GPIO_CH446_VEE 17
#define GPIO_CH446_VOLT 27

// RP led
#define GPIO_LED 25

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
