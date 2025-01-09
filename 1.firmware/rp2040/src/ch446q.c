#include <stdint.h>
#include "ch446q.h"
#include "hardware/gpio.h"
#include "pindef.h"
#include "utils.h"
#include "storage.h"
#include <string.h>

/*                ---------------------
 *    CANL ----- | Y0               X0 | ----- OBD_P1
 *    CANH ----- | Y1               X1 | ----- OBD_P2
 *    CANL ----- | Y2               X2 | ----- OBD_P3
 *    CANH ----- | Y3               X3 | ----- OBD_P4
 *    CANL ----- | Y4               X4 | ----- OBD_P5
 *    CANH ----- | Y5               X5 | ----- OBD_P6
 *    CANL ----- | Y6               X6 | ----- OBD_P11
 *    CANH ----- | Y7               X7 | ----- OBD_P12
 *           --- | AX0              X8 | ----- OBD_P13
 *          |--- | AX1              X9 | ----- OBD_P14
 *          |--- | AX2             X10 | ----- OBD_P15
 *   GND----|--- | AX3             X11 | ----- OBD_P16
 *          |--- | AY0             X12 | ----- OBD_P7
 *          |--- | AY1             X13 | ----- OBD_P8
 *           --- | AY2             X14 | ----- OBD_P9
 *  SW_RST ----- | RST             X15 | ----- OBD_P10
 *  SW_DAT ----- | DAT             VDD | ----- VCC
 *  SW_CLK ----- | CS/CK           P/S | ----- SW_PS
 *  SW_STB ----- | STB             GND | ----- GND
 *         X---- | NC_5            VEE | ----- -5V_VEE
 *         X---- | NC_4           NC_1 | ----X
 *         X---- | NC_3           NC_2 | ----X
 *                ---------------------
 * */

uint8_t PIN_MAP_ARR[16] = {0, 1, 2, 3, 4, 5, 12, 13, 14, 15, 6, 7, 8, 9, 10, 11};
extern uint8_t *g_CAN_high_pin, *g_CAN_low_pin;

void CH446Q_init()
{
  // set up control CH446
  // external manual reset, active at high level
  gpio_init(GPIO_SW_RST);
  gpio_set_dir(GPIO_SW_RST, GPIO_OUT);
  gpio_put(GPIO_SW_RST, 0);

  // P/S: address input mode selection:
  // P/S at high: parallel input mode
  // P/S at low: serial input mode
  gpio_init(GPIO_SW_PS);
  gpio_set_dir(GPIO_SW_PS, GPIO_OUT);
  gpio_put(GPIO_SW_PS, 0); // serial input mode

  // serial clock input in serial address mode, active on rising edges;
  // Chip selection input in parallel address mode, active at high level;
  gpio_init(GPIO_SW_CLK);
  gpio_set_dir(GPIO_SW_CLK, GPIO_OUT);
  gpio_put(GPIO_SW_CLK, 0);

  // serial data input and switch data input in serial address mode
  // Switch data input in parallel address mode, ON at high level, OFF at low level
  gpio_init(GPIO_SW_DAT);
  gpio_set_dir(GPIO_SW_DAT, GPIO_OUT);
  gpio_put(GPIO_SW_DAT, 0);

  // Strobe pulse input, active at high level
  gpio_init(GPIO_SW_STB);
  gpio_set_dir(GPIO_SW_STB, GPIO_OUT);
  gpio_put(GPIO_SW_STB, 0);
  
  // end setup control CH446Q
  // CH446Q_reset();

}

void CH446Q_reset()
{
  gpio_put(GPIO_SW_RST, 1);
  delay_ms(25);
  gpio_put(GPIO_SW_RST, 0);
}


void CH446Q_connect_pins(uint8_t * CAN_high_OBDII_pin, uint8_t * CAN_low_OBDII_pin)
{
  memcpy(g_CAN_high_pin, CAN_high_OBDII_pin, MAX_NUM_PINS * sizeof(uint8_t));
  memcpy(g_CAN_low_pin, CAN_low_OBDII_pin, MAX_NUM_PINS * sizeof(uint8_t));
  //off all other pins by reset
  CH446Q_reset();
  CH446q_refresh();
}


void CH446q_refresh()
{
  if(is_all_zero(g_CAN_high_pin, MAX_NUM_PINS) == 1 
    || is_all_zero(g_CAN_low_pin, MAX_NUM_PINS) == 1)
  {
    return;
  }
  for(int i = 0; i < MAX_NUM_PINS; i++)
  {
    if(g_CAN_high_pin[i] > 0 && g_CAN_high_pin[i] < 16)
    {
      uint8_t CAN_high_pos = PIN_MAP_ARR[g_CAN_high_pin[i] - 1];
      CH446Q_switch_channel(1, CAN_high_pos, 1);
      CH446Q_switch_channel(3, CAN_high_pos, 1);
      CH446Q_switch_channel(5, CAN_high_pos, 1);
      CH446Q_switch_channel(7, CAN_high_pos, 1);
    }
    if(g_CAN_low_pin[i] > 0 && g_CAN_low_pin[i] < 16)
    {
      uint8_t CAN_low_pos = PIN_MAP_ARR[g_CAN_low_pin[i] - 1];
      CH446Q_switch_channel(0, CAN_low_pos, 1);
      CH446Q_switch_channel(2, CAN_low_pos, 1);
      CH446Q_switch_channel(4, CAN_low_pos, 1);
      CH446Q_switch_channel(6, CAN_low_pos, 1);
    }
  }
}

void CH446Q_switch_channel(uint8_t y, uint8_t x, uint8_t status) {
  
    uint32_t chAddress = 0;
    
    int chYdata = y;
    int chXdata = x;

    chYdata = chYdata << 5;
    chYdata = chYdata & 0b11100000;

    chXdata = chXdata << 1;
    chXdata = chXdata & 0b00011110;

    chAddress = chYdata | chXdata;

    if (status == 1)
    {
      chAddress = chAddress | 0b00000001; // this last bit determines whether we set or unset the path
    }

    chAddress = chAddress << 24;


    // Start the transmission by setting STB low
    gpio_put(GPIO_SW_STB, 0);
    gpio_put(GPIO_SW_DAT, 1);
    gpio_put(GPIO_SW_CLK, 1);
    delay_us(100);
    
    // Send the address (7 bits: 4 bits for x, 3 bits for y)
    for (int i = 31; i >= 25; i--) {  
        uint32_t mask = (1 << i);    
        gpio_put(GPIO_SW_CLK, 0);
        if (chAddress & mask) {            
            gpio_put(GPIO_SW_DAT, 1);      
        } else {
            gpio_put(GPIO_SW_DAT, 0); 
        }
        delay_us(25);
        gpio_put(GPIO_SW_CLK, 1);
        delay_us(25);
    }

    // Send the state bit
    gpio_put(GPIO_SW_CLK, 0);
    delay_us(50);
    if(status == 1)
      gpio_put(GPIO_SW_DAT,  1);
    else
      gpio_put(GPIO_SW_DAT,  0);
    delay_us(50);

    // End the transmission by setting STB high
    gpio_put(GPIO_SW_STB, 1);
    delay_us(100);
    gpio_put(GPIO_SW_STB, 0);
}

