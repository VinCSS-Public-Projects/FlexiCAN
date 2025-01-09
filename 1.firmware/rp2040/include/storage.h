#ifndef __STORAGE__H__
#define __STORAGE__H__
#include <stdint.h>
#ifdef __cplusplus
extern "C"
{
#endif

#define MAX_NUM_PROFILE 10
#define STORAGE_SIZE 4096 // size must by align by 4096 (FLASH_SECTOR_SIZE)
#define MAX_PROFILE_NAME_LENGTH 128
#define MAX_STRING_LENGTH 64
#define MAX_NUM_PINS 15

  // what kind of data is stored on flash?
  enum STATUS
  {
    NOTSET,
    ACTIVE,
    INACTIVE
  };

  typedef struct
  {
    uint8_t name[MAX_PROFILE_NAME_LENGTH];
    enum STATUS status;
    uint8_t can_high_pin[MAX_NUM_PINS];
    uint8_t can_low_pin[MAX_NUM_PINS];
  } profile_t;

  // for shell login
  typedef struct
  {
    uint8_t username[MAX_STRING_LENGTH];
    uint8_t password[MAX_STRING_LENGTH];
    uint8_t hostname[MAX_STRING_LENGTH];
  } shell_user_t;

  typedef struct
  {
    shell_user_t shell_user;
    profile_t profiles[MAX_NUM_PROFILE];
    uint8_t num_of_profiles;
    uint8_t is_first_run;
    uint8_t is_enable_120R;
    // padding to keep align
    uint8_t padding[STORAGE_SIZE - (sizeof(shell_user_t) +
                                    sizeof(profile_t) * MAX_NUM_PROFILE +
                                    sizeof(uint8_t))

    ];
  } storage_t __attribute__((aligned(STORAGE_SIZE)));

  /*
   * Flash operations
   * */
  void storage_read(storage_t *out);
  void storage_write(storage_t *in);
  void erase_storage();
  void set_default_storage(storage_t *out);

#ifdef __cplusplus
} /*extern "C"*/
#endif
#endif
