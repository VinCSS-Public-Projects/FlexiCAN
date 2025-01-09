#include "storage.h"
#include "hardware/regs/addressmap.h"
#include "hardware/sync.h"
#include "hardware/flash.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>

/*
 * How many bytes we will use for storage the configuration?
 * W26Q16JV has total 2MB flash
 * So, we will use 1 sector (4KB) for configuration
 * we are going to store data at end of flash memory
 * */
#define FLASH_TARGET_OFFSET 0x1ff000

uint8_t *ptr_flash_storage_start = (uint8_t *)(XIP_BASE + FLASH_TARGET_OFFSET);

void erase_storage()
{
  flash_range_erase(FLASH_TARGET_OFFSET, STORAGE_SIZE);
}

void storage_write(storage_t *in)
{
  uint32_t ints = save_and_disable_interrupts();
  erase_storage();

  flash_range_program(FLASH_TARGET_OFFSET, (uint8_t *)in, STORAGE_SIZE);

  restore_interrupts(ints);
}

void storage_read(storage_t *out)
{
  uint8_t *start = ptr_flash_storage_start;
  uint8_t *ptr = (uint8_t *)out;
  for (uint16_t i = 0; i < STORAGE_SIZE; i++)
  {
    ptr[i] = start[i];
  }
}

void set_default_storage(storage_t *out)
{
  strcpy(out->shell_user.username, "root");
  strcpy(out->shell_user.password, "");
  strcpy(out->shell_user.hostname, "FlexiCAN");
  out->num_of_profiles = 0;
  memset(out->profiles, 0, sizeof(profile_t) * MAX_NUM_PROFILE);
  out->is_first_run = 0;
  out->is_enable_120R = 0;
}
