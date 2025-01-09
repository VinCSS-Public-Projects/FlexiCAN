#include "menu.h"
#include <stdint.h>
#include "hardware/gpio.h"
#include "hardware/sync.h"
#include "hardware/structs/ioqspi.h"
#include "hardware/structs/sio.h"
#include <string.h>
#include "pindef.h"
#include "ssd1306.h"
#include "utils.h"
#include "storage.h"
#include <stdio.h>
#include "ch446q.h"
#include "pico/multicore.h"
#include "pico/bootrom.h"

#define CS_BIT (1u << 1)

bool __no_inline_not_in_flash_func(get_bootsel_button)()
{
  const uint CS_PIN_INDEX = 1;

  // Must disable interrupts, as interrupt handlers may be in flash, and we
  // are about to temporarily disable flash access!
  uint32_t flags = save_and_disable_interrupts();

  // Set chip select to Hi-Z
  hw_write_masked(&ioqspi_hw->io[CS_PIN_INDEX].ctrl,
                  GPIO_OVERRIDE_LOW << IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_LSB,
                  IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_BITS);

  // Note we can't call into any sleep functions in flash right now
  for (volatile int i = 0; i < 1000; ++i)
    ;

  // The HI GPIO registers in SIO can observe and control the 6 QSPI pins.
  // Note the button pulls the pin *low* when pressed.
  bool button_state = !(sio_hw->gpio_hi_in & CS_BIT);

  // Need to restore the state of chip select, else we are going to have a
  // bad time when we return to code in flash!
  hw_write_masked(&ioqspi_hw->io[CS_PIN_INDEX].ctrl,
                  GPIO_OVERRIDE_NORMAL << IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_LSB,
                  IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_BITS);

  restore_interrupts(flags);

  return button_state;
}
char current_selection[256];
char current_pos = 0;
#define NUM_OF_ITEM 4
char menu_text[NUM_OF_ITEM][256] = {
    "1. Enable/disable 120R",
    "2. Reset factory",
    "3. Reboot DFU",
    "4. About"};
uint32_t set = 0;
uint32_t count = 0;
uint8_t menu_state = 0;
uint8_t y_axis = 0;
uint8_t active_k1_status = 0;

void active_k1_button()
{
  gpio_init(GPIO_BUTTON_K1);
  gpio_set_dir(GPIO_BUTTON_K1, GPIO_OUT);
}
void deactive_k1_button()
{
  gpio_deinit(GPIO_BUTTON_K1);
  // gpio_pull_down(GPIO_BUTTON_K1);
}

void notification()
{
  extern ssd1306_t g_display;
  delay_ms(100);
  ssd1306_clear(&g_display);
  ssd1306_draw_string(&g_display, 1, 10, 1, "CH446Q setting.");
  ssd1306_draw_string(&g_display, 1, 20, 1, "Refreshing ...");
  ssd1306_show(&g_display);
}
void notification_done()
{
  extern ssd1306_t g_display;
  ssd1306_clear(&g_display);
  ssd1306_draw_string(&g_display, 1, 10, 1, "CH446Q setting.");
  ssd1306_draw_string(&g_display, 1, 20, 1, "Refreshed.");
  ssd1306_show(&g_display);
  delay_ms(500);
}
void notification_reboot(){
  extern ssd1306_t g_display;
  ssd1306_clear(&g_display);
  ssd1306_draw_string(&g_display, 1, 10, 1, "DFU: ready to use");
  ssd1306_draw_string(&g_display, 1, 25, 1, ">Restart to exit DFU");
  ssd1306_show(&g_display);
}
void menu()
{
  extern ssd1306_t g_display;
  extern storage_t g_storage;
  extern uint8_t g_reload_CH446Q_setting;
  uint8_t i;
  uint8_t active_id = 0;
  char buf[128];
  char R120[5];
  switch (menu_state)
  {

  case 0:
  {
    // show current active profiles
    if (g_storage.num_of_profiles > 0)
    {
      for (i = 0; i < g_storage.num_of_profiles; i++)
      {
        if (g_storage.profiles[i].status == ACTIVE)
        {
          active_id = i;
          break;
        }
      }
      ssd1306_clear(&g_display);
      if (g_storage.is_enable_120R)
      {
        strcpy(R120, "120R");
      }
      else
      {
        strcpy(R120, "");
      }

      sprintf(buf, "High:%2d|Low:%2d|%5s", g_storage.profiles[active_id].can_high_pin[0], 
                    g_storage.profiles[active_id].can_low_pin[0], R120);
      ssd1306_draw_string(&g_display, 1, 0, 1, buf);
      if(active_k1_status)
      {
        sprintf(buf, "<> %s", g_storage.profiles[active_id].name);
        ssd1306_draw_string(&g_display, 1, 12, 2, buf);
      }else{
        ssd1306_draw_string(&g_display, 1, 12, 2, g_storage.profiles[active_id].name);
      }
      ssd1306_show(&g_display);
    }
    else
    {
      ssd1306_clear(&g_display);
      ssd1306_draw_string(&g_display, 1, 10, 1, "Please add a profile");
      ssd1306_draw_string(&g_display, 1, 22, 1, "to start.");
      ssd1306_show(&g_display);
    }
    if(active_k1_status)
    {
      if (gpio_get(GPIO_BUTTON_K1))
      {
        set = 0;
        count = 0;
        while (gpio_get(GPIO_BUTTON_K1))
        {
          count++;
          delay_us(100);
        }
        if (count < 500)
        {
          if (set == 0)
          {
            active_id = (++active_id % g_storage.num_of_profiles);
            for (i = 0; i < g_storage.num_of_profiles; i++)
            {
              if (i == active_id)
              {
                g_storage.profiles[active_id].status = ACTIVE;
                g_reload_CH446Q_setting = 1;
              }
              else
              {
                g_storage.profiles[i].status = INACTIVE;
              }
            }
            storage_write(&g_storage);

            set = 1;
          }
        }
      }
    }
    break;
  }
  case 1:
  {
    current_pos = current_pos >= NUM_OF_ITEM ? current_pos % NUM_OF_ITEM : current_pos;
    strcpy(current_selection, menu_text[current_pos]);
    ssd1306_clear(&g_display);
    ssd1306_draw_string(&g_display, 1, 10, 1, current_selection);
    ssd1306_show(&g_display);
    if(active_k1_status)
    {
      if (gpio_get(GPIO_BUTTON_K1))
      {
        set = 0;
        count = 0;
        while (gpio_get(GPIO_BUTTON_K1))
        {
          count++;
          delay_us(100);
        }
        if (count > 500)
        {
          if (set == 0)
          {
            switch (current_pos)
            {
              case 0:
              {
                g_storage.is_enable_120R = g_storage.is_enable_120R == 0 ? 1 : 0;
                storage_write(&g_storage);
                menu_state = 2;
                break;
              }
              case 1: //reset factory
              {
                menu_state = 3;
                set_default_storage(&g_storage);
                storage_write(&g_storage);
                break;
              }
              case 2: //reboot dfu
              {
                menu_state = 4;
                notification_reboot();
                reset_usb_boot(0, 0);
                break;
              }
              case 3: //about
              {
                menu_state = 5;
                y_axis = 0;
                break;
              }
              default:
                  break;

            }
            set = 1;
          }
        }
        else if (count < 500)
        {
          if (set == 0)
          {
            current_pos++;
            set = 1;
          }
        }
      }
    }
    break;
  }
  case 2:
  {
    if (g_storage.is_enable_120R)
    {
      strcpy(buf, "Enabled 120 Ohm resistor");
    }
    else
    {
      strcpy(buf, "Disabled 120 Ohm resistor");
    }
    ssd1306_clear(&g_display);
    ssd1306_draw_string(&g_display, 1, 10, 1, buf);
    ssd1306_show(&g_display);
    break;
  }
  case 3: //reset factory
  {
    ssd1306_clear(&g_display);
    ssd1306_draw_string(&g_display, 1, 10, 1, "Reset factory");
    ssd1306_show(&g_display);

    break;
  }

  case 4: //reboot dfu
  {
    ssd1306_clear(&g_display);
    ssd1306_draw_string(&g_display, 1, 10, 1, "Reboot DFU");
    ssd1306_show(&g_display);
    reset_usb_boot(0, 0);

    break;
  }

  case 5: //about
  {
    char about[10][32] = {
          "FlexiCAN",
          "A CAN device",
          "Developed by VinCSS.",
          "-------------------",
          "Authors:",
          "1. Chu Tien Thinh",
          "2. Nguyen Van Cuong",
          "Website:",
          "https://vincss.net",
          "--------END--------"
        };

    ssd1306_clear(&g_display);
    int tmp = 0;
    y_axis++;
    for(i = 0; i < 10; i++){
      tmp = (i+1) * 10 - y_axis;
      if(tmp > 0)
      {
        ssd1306_draw_string(&g_display, 1, tmp, 1, about[i]);
      }
    }
    if( 11 * 10 - y_axis < 10)
    {
      y_axis = 0;
    }
    ssd1306_show(&g_display);
    delay_ms(10);
    break;
  }
  default:
    break;
  }

  // //draw button status
  // if(active_k1_status == 1 && menu_state == 0)
  // {
  //   ssd1306_draw_string(&g_display, 1, 27, 1, "XXXX");
  //   ssd1306_show(&g_display);
  // }

  //check button status
  if (get_bootsel_button())
  {
    set = 0;
    count = 0;
    while (get_bootsel_button())
    {
      count++;
      delay_us(100);
    }

    if(count > 500 && active_k1_status == 0 && menu_state == 0)
    {
      active_k1_status = 1;
      active_k1_button();
    }else if(menu_state == 0 && active_k1_status == 1){
      active_k1_status = 0;
      deactive_k1_button();
      if(g_reload_CH446Q_setting)
      {
        notification();
        CH446Q_connect_pins(g_storage.profiles[active_id].can_high_pin, g_storage.profiles[active_id].can_low_pin);
        notification_done();
        g_reload_CH446Q_setting = 0;
      }
    }else if(menu_state == 0 && active_k1_status == 0 && count <= 500){
      if (set == 0)
      {
        menu_state = 1;
        set = 1;
        current_pos = 0;
        if(active_k1_status == 0)
        {
          active_k1_status = 1;
          active_k1_button();
        }
      }
    }else if(menu_state != 0){
      menu_state = 0;
      active_k1_status = 0;
      deactive_k1_button();
    }
  }

  if(active_k1_status == 0 && menu_state == 0)
  {
    if(g_reload_CH446Q_setting)
    {
      notification();
      CH446Q_connect_pins(g_storage.profiles[active_id].can_high_pin, g_storage.profiles[active_id].can_low_pin);
      notification_done();
      g_reload_CH446Q_setting = 0;
    }
  }
}
