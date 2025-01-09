#include "hardware/gpio.h"
#include "hardware/structs/io_bank0.h"
#include "pico/binary_info.h"
#include "hardware/i2c.h"
#include "pindef.h"
#include "utils.h"
#include "ssd1306.h"
#include "storage.h"
#include "shell.h"
#include "menu.h"
#include "ch446q.h"
#include <string.h>

/*
 *  Logical Diagram
 *
 *         [Computer]
 *              ^
 *              |
 *    ------>[USB Hub]
 *   |          |
 *   |          |  _____________________________________
 *   |          | |                                     |
 *   |          | |                                     |
 *   |          | |  [OLED Display]                     |
 *   |          | |      /                              |
 *   |          | |     /                               |
 *   |          | |   I2C                               |
 *   |          | |   /                                 |
 *   |          v v  /                               GPIO control
 *   |      ->[RP2040] <-- SPI --> [Flash 2M]           |
 *   |     |     ^    \                                 |
 *   |     |     |     \                                |
 *   |   BOOT   I2C     \                               |
 *   |     |     |       \                              |
 *   |     v     v        \                             v
 *    --->[STM32...]<-->[CAN Transceiver] <--------> [CH446Q] <----> [OBD-II]
 *
 * */

// global variables
ssd1306_t g_display;
storage_t g_storage;
uint8_t g_reload_CH446Q_setting = 1;
uint8_t g_CAN_high_pin[MAX_NUM_PINS] = {0}, g_CAN_low_pin[MAX_NUM_PINS] = {0};


// functions
void init_default_pins()
{
  // setup control boot for STM32
  // Set STM32 PB8_BOOT0 to 0 to enable STM32 boot from its main Flash Memory
  gpio_init(GPIO_RP_STM_BOOT0);
  gpio_put(GPIO_RP_STM_BOOT0, 0);
  gpio_pull_down(GPIO_RP_STM_BOOT0);
  gpio_set_dir(GPIO_RP_STM_BOOT0, GPIO_OUT);
  gpio_put(GPIO_RP_STM_BOOT0, 0);

  gpio_init(GPIO_RP_STM_RST);
  gpio_set_dir(GPIO_RP_STM_RST, GPIO_OUT);
  gpio_put(GPIO_RP_STM_RST, 1);
  delay_ms(2);
  // keep stm32 reset
  gpio_put(GPIO_RP_STM_RST, 0);
  delay_ms(2);

  gpio_deinit(GPIO_RP_STM_BOOT0);
  // end setup control dfu mode of STM32

  // set CAN TX/RX for RP2040. 0 = RP2040, 1 = STM32
  gpio_init(GPIO_CAN_SOURCE);
  gpio_set_dir(GPIO_CAN_SOURCE, GPIO_OUT); 
  gpio_put(GPIO_CAN_SOURCE, 1);
  
  gpio_init(GPIO_CAN_HACK_RX);
  gpio_init(GPIO_CAN_HACK_TX);
  gpio_set_dir(GPIO_CAN_HACK_RX, GPIO_IN);// just set GPIO_IN to avoid any inferrence to CAN. If we need send out data, need to re-initilize it to GPIO_OUT
  gpio_set_dir(GPIO_CAN_HACK_TX, GPIO_IN);
  // end CAN TX/RX for RP2040

  //start STM32 again
  gpio_put(GPIO_RP_STM_RST, 1);
  delay_ms(2);
  gpio_set_dir(GPIO_RP_STM_RST, GPIO_IN);
  gpio_pull_up(GPIO_RP_STM_RST);
}

void init_oled_display()
{
  // baurate 100000
  i2c_init(GPIO_I2C_PORT, 100 * 1000);
  gpio_set_function(GPIO_I2C_SDA, GPIO_FUNC_I2C);
  gpio_set_function(GPIO_I2C_SCL, GPIO_FUNC_I2C);
  gpio_pull_up(GPIO_I2C_SDA);
  gpio_pull_up(GPIO_I2C_SCL);
  bi_decl(bi_2pins_with_func(GPIO_I2C_SDA, GPIO_I2C_SCL, GPIO_FUNC_I2C));
  g_display.external_vcc = 0;
  ssd1306_init(&g_display, OLED_WIDTH, OLED_HEIGHT, I2C_ADDR, GPIO_I2C_PORT);
  ssd1306_clear(&g_display);
}

void init_storage()
{
  storage_read(&g_storage);
}

void init_CH446Q()
{
  CH446Q_init();
}

void logo()
{
  ssd1306_clear(&g_display);
  ssd1306_draw_string(&g_display, 7, 10, 3, "VinCSS");
  ssd1306_show(&g_display);
}

void load_current_CAN_config()
{
  for (int i = 0; i < g_storage.num_of_profiles; i++)
  {
    if (g_storage.profiles[i].status == ACTIVE)
    {
      memcpy(g_CAN_high_pin, g_storage.profiles[i].can_high_pin, 
                    MAX_NUM_PINS * sizeof(uint8_t));
      memcpy(g_CAN_low_pin, g_storage.profiles[i].can_low_pin, 
                    MAX_NUM_PINS * sizeof(uint8_t));
      break;
    }
  }
}

int main()
{
  uint32_t refresh_counter = 0;
  init_default_pins();
  init_oled_display();
  init_storage();
  init_CH446Q();

  if (g_storage.is_first_run)
  {
    set_default_storage(&g_storage);
    storage_write(&g_storage);
  }

  shell_init(&g_storage);

  logo();
  delay_ms(1000);
  //load current active profile;
  load_current_CAN_config();
  CH446Q_reset();
  CH446q_refresh();
  
  while (1)
  {
    shell_main();
    menu();
    refresh_counter++;
    delay_us(100);
    if(refresh_counter % 1000000){ //100s refresh one time
      CH446q_refresh();
      refresh_counter++;
    }
  }
}
