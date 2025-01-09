#include <stdio.h>
#include <stdbool.h>
#include "csh.h"
#include "chry_ringbuffer.h"

#include "usbd_core.h"
#include "usbd_cdc.h"
#include "storage.h"
#include "utils.h"
#include "pico/bootrom.h"

#ifdef __cplusplus
extern "C"
{
#endif

  static chry_shell_t csh;
  static bool login = false;
  static chry_ringbuffer_t shell_rb;
  static uint8_t mempool[1024];

/*!< endpoint address */
#define CDC_IN_EP 0x81
#define CDC_OUT_EP 0x04
#define CDC_INT_EP 0x83

#define USBD_VID 0xFFFF
#define USBD_PID 0xFFFF
#define USBD_MAX_POWER 100
#define USBD_LANGID_STRING 1033

/*!< config descriptor size */
#define USB_CONFIG_SIZE (9 + CDC_ACM_DESCRIPTOR_LEN)

  /*!< global descriptor */
  static const uint8_t cdc_descriptor[] = {
      USB_DEVICE_DESCRIPTOR_INIT(USB_2_0, 0xEF, 0x02, 0x01, USBD_VID, USBD_PID, 0x0100, 0x01),
      USB_CONFIG_DESCRIPTOR_INIT(USB_CONFIG_SIZE, 0x02, 0x01, USB_CONFIG_BUS_POWERED, USBD_MAX_POWER),
      CDC_ACM_DESCRIPTOR_INIT(0x00, CDC_INT_EP, CDC_OUT_EP, CDC_IN_EP, 0x02),
      ///////////////////////////////////////
      /// string0 descriptor
      ///////////////////////////////////////
      USB_LANGID_INIT(USBD_LANGID_STRING),
      ///////////////////////////////////////
      /// string1 descriptor
      ///////////////////////////////////////
      0x18,                       /* bLength */
      USB_DESCRIPTOR_TYPE_STRING, /* bDescriptorType */
      'V', 0x00,                  /* wcChar0 */
      'i', 0x00,                  /* wcChar1 */
      'n', 0x00,                  /* wcChar2 */
      'C', 0x00,                  /* wcChar3 */
      'S', 0x00,                  /* wcChar4 */
      'S', 0x00,                  /* wcChar5 */
      ',', 0x00,                  /* wcChar6 */
      'J', 0x00,                  /* wcChar6 */
      'S', 0x00,                  /* wcChar6 */
      'C', 0x00,                  /* wcChar6 */
      '.', 0x00,                  /* wcChar6 */
      ///////////////////////////////////////
      /// string2 descriptor
      ///////////////////////////////////////
      0x28,                       /* bLength */
      USB_DESCRIPTOR_TYPE_STRING, /* bDescriptorType */
      'F', 0x00,                  /* wcChar0 */
      'l', 0x00,                  /* wcChar1 */
      'e', 0x00,                  /* wcChar2 */
      'x', 0x00,                  /* wcChar3 */
      'i', 0x00,                  /* wcChar4 */
      'C', 0x00,                  /* wcChar5 */
      'A', 0x00,                  /* wcChar6 */
      'N', 0x00,                  /* wcChar7 */
      ' ', 0x00,                  /* wcChar8 */
      'C', 0x00,                  /* wcChar9 */
      'o', 0x00,                  /* wcChar10 */
      'n', 0x00,                  /* wcChar11 */
      't', 0x00,                  /* wcChar12 */
      'r', 0x00,                  /* wcChar13 */
      'o', 0x00,                  /* wcChar14 */
      'l', 0x00,                  /* wcChar15 */
      'l', 0x00,                  /* wcChar16 */
      'e', 0x00,                  /* wcChar17 */
      'r', 0x00,                  /* wcChar19 */
      ///////////////////////////////////////
      /// string3 descriptor
      ///////////////////////////////////////
      0x1c,                       /* bLength */
      USB_DESCRIPTOR_TYPE_STRING, /* bDescriptorType */
      'V', 0x00,
      'i', 0x00,
      'n', 0x00,
      'C', 0x00, /* wcChar0 */
      'S', 0x00, /* wcChar1 */
      'S', 0x00, /* wcChar2 */
      '0', 0x00, /* wcChar3 */
      '0', 0x00, /* wcChar4 */
      '0', 0x00, /* wcChar5 */
      '0', 0x00, /* wcChar6 */
      '0', 0x00, /* wcChar7 */
      '0', 0x00, /* wcChar8 */
      '1', 0x00, /* wcChar9 */
#ifdef CONFIG_USB_HS
      ///////////////////////////////////////
      /// device qualifier descriptor
      ///////////////////////////////////////
      0x0a,
      USB_DESCRIPTOR_TYPE_DEVICE_QUALIFIER,
      0x00,
      0x02,
      0x02,
      0x02,
      0x01,
      0x40,
      0x01,
      0x00,
#endif
      0x00};

  USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t read_buffer[2048];
  USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t write_buffer[2048];

  volatile bool ep_tx_busy_flag = false;

#define CDC_MAX_MPS 64

  void usbd_configure_done_callback(void)
  {
    /* setup first out ep read transfer */
    usbd_ep_start_read(CDC_OUT_EP, read_buffer, 2048);
  }

  void usbd_cdc_acm_bulk_out(uint8_t ep, uint32_t nbytes)
  {
    chry_ringbuffer_write(&shell_rb, read_buffer, nbytes);
    usbd_ep_start_read(CDC_OUT_EP, read_buffer, 2048);
  }

  void usbd_cdc_acm_bulk_in(uint8_t ep, uint32_t nbytes)
  {
    USB_LOG_RAW("actual in len:%d\r\n", nbytes);

    if ((nbytes % CDC_MAX_MPS) == 0 && nbytes)
    {
      /* send zlp */
      usbd_ep_start_write(CDC_IN_EP, NULL, 0);
    }
    else
    {
      ep_tx_busy_flag = false;
    }
  }

  /*!< endpoint call back */
  struct usbd_endpoint cdc_out_ep = {
      .ep_addr = CDC_OUT_EP,
      .ep_cb = usbd_cdc_acm_bulk_out};

  struct usbd_endpoint cdc_in_ep = {
      .ep_addr = CDC_IN_EP,
      .ep_cb = usbd_cdc_acm_bulk_in};

  struct usbd_interface intf0;
  struct usbd_interface intf1;

  void cdc_acm_init(void)
  {

    // const uint8_t data[10] = { 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30 };

    // memcpy(&write_buffer[0], data, 10);
    // memset(&write_buffer[10], 'a', 2038);

    usbd_desc_register(cdc_descriptor);
    usbd_add_interface(usbd_cdc_acm_init_intf(&intf0));
    usbd_add_interface(usbd_cdc_acm_init_intf(&intf1));
    usbd_add_endpoint(&cdc_out_ep);
    usbd_add_endpoint(&cdc_in_ep);
    usbd_initialize();

    // Wait until configured
    while (!usb_device_is_configured())
    {
      // tight_loop_contents();
    }
  }

  volatile uint8_t dtr_enable = 0;

  void usbd_cdc_acm_set_dtr(uint8_t intf, bool dtr)
  {
    if (dtr)
    {
      dtr_enable = 1;
    }
    else
    {
      dtr_enable = 0;
    }
  }

  void cdc_acm_data_send_with_dtr_test(void)
  {
    if (dtr_enable)
    {
      ep_tx_busy_flag = true;
      usbd_ep_start_write(CDC_IN_EP, write_buffer, 2048);
      while (ep_tx_busy_flag)
      {
      }
    }
  }

  static uint16_t csh_sput_cb(chry_readline_t *rl, const void *data, uint16_t size)
  {
    (void)rl;

    memcpy(write_buffer, data, size);
    usbd_ep_start_write(CDC_IN_EP, write_buffer, size);
    if (login)
    {
      ep_tx_busy_flag = true;
      while (ep_tx_busy_flag)
      {
      }
    }
    return size;
  }

  static uint16_t csh_sget_cb(chry_readline_t *rl, void *data, uint16_t size)
  {
    (void)rl;
    return chry_ringbuffer_read(&shell_rb, data, size);
  }

  int shell_init(storage_t *storage)
  {

    cdc_acm_init();

    chry_shell_init_t csh_init;

    if (chry_ringbuffer_init(&shell_rb, mempool, sizeof(mempool)))
    {
      return -1;
    }

    //    if (strcmp(storage->shell_user.password, "") == 0) {
    //        login = true;
    //    } else {
    //        login = false;
    //    }
    //
    login = false;

    /*!< I/O callback */
    csh_init.sput = csh_sput_cb;
    csh_init.sget = csh_sget_cb;

#if defined(CONFIG_CSH_SYMTAB) && CONFIG_CSH_SYMTAB
    extern const int __fsymtab_start;
    extern const int __fsymtab_end;
    extern const int __vsymtab_start;
    extern const int __vsymtab_end;

    /*!< get table from ld symbol */
    csh_init.command_table_beg = &__fsymtab_start;
    csh_init.command_table_end = &__fsymtab_end;
    csh_init.variable_table_beg = &__vsymtab_start;
    csh_init.variable_table_end = &__vsymtab_end;
#endif

#if defined(CONFIG_CSH_PROMPTEDIT) && CONFIG_CSH_PROMPTEDIT
    static char csh_prompt_buffer[128];

    /*!< set prompt buffer */
    csh_init.prompt_buffer = csh_prompt_buffer;
    csh_init.prompt_buffer_size = sizeof(csh_prompt_buffer);
#endif

#if defined(CONFIG_CSH_HISTORY) && CONFIG_CSH_HISTORY
    static char csh_history_buffer[4096]; // this buffer has greater than line buffer below. Otherwise, it will be crashed.

    /*!< set history buffer */
    csh_init.history_buffer = csh_history_buffer;
    csh_init.history_buffer_size = sizeof(csh_history_buffer);
#endif

#if defined(CONFIG_CSH_LNBUFF_STATIC) && CONFIG_CSH_LNBUFF_STATIC
    static char csh_line_buffer[128];

    /*!< set linebuffer */
    csh_init.line_buffer = csh_line_buffer;
    csh_init.line_buffer_size = sizeof(csh_line_buffer);
#endif

    csh_init.uid = 0;
    csh_init.user[0] = storage->shell_user.username;

    /*!< The port hash function is required,
         and the strcmp attribute is used weakly by default,
         int chry_shell_port_hash_strcmp(const char *hash, const char *str); */
    csh_init.hash[0] = login ? NULL : storage->shell_user.password; /*!< If there is no password, set to NULL */
    csh_init.host = storage->shell_user.hostname;
    csh_init.user_data = NULL;

    int ret = chry_shell_init(&csh, &csh_init);
    if (ret)
    {
      return -1;
    }

    return 0;
  }

  int shell_main(void)
  {
    int ret;

  restart:

    if (login)
    {
      goto restart2;
    }

    ret = csh_login(&csh);
    if (ret == 0)
    {
      login = true;
    }
    else
    {
      return 0;
    }

  restart2:
    chry_shell_task_exec(&csh);

    ret = chry_shell_task_repl(&csh);

    if (ret == -1)
    {
      /*!< error */
      return -1;
    }
    else if (ret == 1)
    {
      /*!< continue */
      return 0;
    }
    else
    {
      /*!< restart */
      goto restart;
    }

    return 0;
  }

  void shell_lock(void)
  {
    chry_readline_erase_line(&csh.rl);
  }

  void shell_unlock(void)
  {
    chry_readline_edit_refresh(&csh.rl);
  }

  static int csh_exit(int argc, char **argv)
  {
    (void)argc;
    (void)argv;
    login = false;
    return 0;
  }
  CSH_SCMD_EXPORT_ALIAS(csh_exit, exit, );

  // FlexiCANshell command
  static int list_profiles(int argc, char **argv)
  {
    chry_shell_t *sh = (chry_shell_t *)argv[argc + 1];
    if ((strncmp(argv[1], "help", 4) == 0) ||
        (strcmp(argv[1], "--help") == 0) ||
        (strcmp(argv[1], "-h") == 0))
    {
      csh_printf(sh, "List all profiles in system\r\n");
    }
    else
    {
      extern storage_t g_storage;
      if (g_storage.num_of_profiles == 0)
      {
        csh_printf(sh, "No profiles is available!\r\n");
        return 0;
      }
      char buf[55] = {0};
      char CAN_high_list[55];
      char CAN_low_list[55];
      memset(buf, '-', 54);
      csh_printf(sh, "%s\r\n", buf);
      csh_printf(sh, "|%3s|%20s|%8s|%8s|%10s|\r\n", "ID", "Name", "CAN high", "CAN low", "Status");
      csh_printf(sh, "%s\r\n", buf);
      for (int i = 0; i < g_storage.num_of_profiles; i++)
      {
        array_to_string(CAN_high_list, 55, g_storage.profiles[i].can_high_pin, MAX_NUM_PINS);
        array_to_string(CAN_low_list, 55, g_storage.profiles[i].can_low_pin, MAX_NUM_PINS);
        csh_printf(sh, "|%3d|%20s|%8s|%8s|%10s|\r\n", i, g_storage.profiles[i].name, CAN_high_list, CAN_low_list, g_storage.profiles[i].status == 1 ? "active" : "not active");
        csh_printf(sh, "%s\r\n", buf);
      }
    }

    return 0;
  }
  CSH_CMD_EXPORT(list_profiles, );

  static int add_profile(int argc, char **argv)
  {
    chry_shell_t *sh = (chry_shell_t *)argv[argc + 1];
    if (
        (argc < 4) ||
        (strncmp(argv[1], "help", 4) == 0) ||
        (strcmp(argv[1], "--help") == 0) ||
        (strcmp(argv[1], "-h") == 0))
    {
      csh_printf(sh, "Add profile\r\n%s <PROFILE NAME> <CAN HIGH PIN List> <CAN LOW PIN list> <ACTIVE|NOTACTIVE>\r\nDefault profile is not active.\r\nPIN in PIN list are separated by comma\r\nExample: %s Test_Profile 3,5 6,7\r\n", argv[0],argv[0]);
    }
    else
    {
      extern storage_t g_storage;
      extern uint8_t g_reload_CH446Q_setting;
      // validation
      if (strlen(argv[1]) >= MAX_PROFILE_NAME_LENGTH)
      {
        csh_printf(sh, "Profile name is too long.\r\nMaximum length should be %d\r\n", MAX_PROFILE_NAME_LENGTH);
        return -1;
      }

      uint8_t CAN_high_pin[MAX_NUM_PINS]; // = atoi(argv[2]);
      uint8_t CAN_low_pin[MAX_NUM_PINS]; // = atoi(argv[3]);
      
      if((extract_pins(argv[2], CAN_high_pin) < 0) || 
          (extract_pins(argv[3], CAN_low_pin) < 0))
      {
        csh_printf(sh, "CAN HIGH PIN or CAN LOW PIN must be in range from 1 to 15\r\n");
        return -1;
      }

      if(is_dupplicated_value(CAN_high_pin, CAN_low_pin, MAX_NUM_PINS))
      {
        csh_printf(sh, "CAN HIGH PIN can not be same as CAN LOW PIN\r\n");
        return -1;
      }

      // if (CAN_high_pin == CAN_low_pin)
      // {
      //   csh_printf(sh, "CAN HIGH PIN can not be same as CAN LOW PIN\r\n");
      //   return -1;
      // }

      // // OBD-II has 16 pin. Pin assign from 1 -> 16. 16th Pin is VCC (12Volt)
      // if (CAN_high_pin >= 16 || CAN_low_pin >= 16 || CAN_high_pin <= 0 || CAN_low_pin <= 0)
      // {
      //   csh_printf(sh, "CAN HIGH PIN or CAN LOW PIN must be in range from 1 to 15\r\n");
      //   return -1;
      // }

      enum STATUS status = INACTIVE;
      if (argc >= 5)
      {
        if (strncmp(str_to_upper(argv[4]), "ACTIVE", 6) == 0)
        {
          status = ACTIVE;
          g_reload_CH446Q_setting = 1;
          for (int i = 0; i < g_storage.num_of_profiles; i++)
          {
            g_storage.profiles[i].status = INACTIVE;
          }
        }
      }

      if(g_storage.num_of_profiles == 0)
      {
        status = ACTIVE;
        g_reload_CH446Q_setting = 1;
      }

      if (g_storage.num_of_profiles >= MAX_NUM_PROFILE)
      {
        csh_printf(sh, "No empty slot to add new profile. Please remove one and add again!\r\n");
        return -1;
      }

      strcpy(g_storage.profiles[g_storage.num_of_profiles].name, argv[1]);
      memcpy(g_storage.profiles[g_storage.num_of_profiles].can_high_pin, CAN_high_pin, sizeof(uint8_t) * MAX_NUM_PINS);
      memcpy(g_storage.profiles[g_storage.num_of_profiles].can_low_pin, CAN_low_pin, sizeof(uint8_t) * MAX_NUM_PINS);
      g_storage.profiles[g_storage.num_of_profiles].status = status;

      g_storage.num_of_profiles++;

      storage_write(&g_storage);
      csh_printf(sh, "Profile added successful!\r\n");
      return 0;
    }

    return 0;
  }
  CSH_CMD_EXPORT(add_profile, );

  static int remove_profile(int argc, char **argv)
  {
    chry_shell_t *sh = (chry_shell_t *)argv[argc + 1];
    int i;

    if (
        (argc != 2) ||
        (strncmp(argv[1], "help", 4) == 0) ||
        (strcmp(argv[1], "--help") == 0) ||
        (strcmp(argv[1], "-h") == 0))
    {
      csh_printf(sh, "Remove a profile in device\r\n%s <ID>\r\n", argv[0]);
    }
    else
    {
      int id = atoi(argv[1]);
      extern storage_t g_storage;
      extern uint8_t g_reload_CH446Q_setting;
      if (id < 0 || id >= g_storage.num_of_profiles)
      {
        csh_printf(sh, "ID is not existed!\r\n");
        return 0;
      }

      char name[MAX_PROFILE_NAME_LENGTH];
      strcpy(name, g_storage.profiles[id].name);
      enum STATUS status = g_storage.profiles[id].status;

      if (id == (g_storage.num_of_profiles - 1))
      {
        memset(&(g_storage.profiles[id]), 0, sizeof(profile_t));
      }
      else
      {
        for (i = id; i < g_storage.num_of_profiles - 1; i++)
        {
          memcpy(&(g_storage.profiles[i]), &(g_storage.profiles[i + 1]), sizeof(profile_t));
        }
        memset(&(g_storage.profiles[g_storage.num_of_profiles - 1]), 0, sizeof(profile_t));
      }
      g_storage.num_of_profiles--;
      if(status == ACTIVE && g_storage.num_of_profiles > 0)
      {
        g_storage.profiles[0].status = ACTIVE;
        g_reload_CH446Q_setting = 1;
      }

      storage_write(&g_storage);
      csh_printf(sh, "Remove %d:%s successful\r\n", id, name);
    }

    return 0;
  }
  CSH_CMD_EXPORT(remove_profile, );

  static int change_profile(int argc, char **argv)
  {
    chry_shell_t *sh = (chry_shell_t *)argv[argc + 1];
    if (
        (argc < 3) ||
        (strncmp(argv[1], "help", 4) == 0) ||
        (strcmp(argv[1], "--help") == 0) ||
        (strcmp(argv[1], "-h") == 0))
    {
      csh_printf(sh, "Change profiles\r\n%s <ID> --name=<NAME> --can-low=<CAN_LOW_PIN> --can-high=<CAN_HIGH_PIN>\r\n", argv[0]);
    }
    else
    {
      int id = atoi(argv[1]);
      extern storage_t g_storage;
      if (id < 0 || id >= g_storage.num_of_profiles)
      {
        csh_printf(sh, "ID is not existed!\r\n");
        return 0;
      }

      char *token;
      uint8_t CAN_pins[MAX_NUM_PINS] = {0};

      for (int i = 2; i < argc; i++)
      {
        token = strtok(argv[i], "=");
        if (token == NULL)
        {
          continue;
        }
        if (strcmp(token, "--name") == 0)
        {
          // process name
          token = strtok(NULL, "=");
          strcpy(g_storage.profiles[id].name, token);
        }
        else if (strcmp(token, "--can-low") == 0)
        {
          token = strtok(NULL, "=");
          memset(CAN_pins, 0,sizeof(CAN_pins));
          if(extract_pins(argv[2], CAN_pins) < 0)
          {
            csh_printf(sh, "PIN must in range between 1 and 15\r\n");
          }
          memcpy(g_storage.profiles[id].can_low_pin, CAN_pins, sizeof(CAN_pins));
        }
        else if (strcmp(token, "--can-high") == 0)
        {
          token = strtok(NULL, "=");
          memset(CAN_pins, 0,sizeof(CAN_pins));
          if(extract_pins(argv[2], CAN_pins) < 0)
          {
            csh_printf(sh, "PIN must in range between 1 and 15\r\n");
          }
          memcpy(g_storage.profiles[id].can_high_pin, CAN_pins, sizeof(CAN_pins));
        }
      }
      if(is_dupplicated_value(g_storage.profiles[id].can_high_pin, g_storage.profiles[id].can_low_pin, MAX_NUM_PINS) == 1)
      {
        csh_printf(sh, "CAN high pin and CAN low pin can not be the same\r\n");
        storage_read(&g_storage); //restore information from flash.
        return -1;
      }

      storage_write(&g_storage);
      extern uint8_t g_reload_CH446Q_setting;
      if(g_storage.profiles[id].status == ACTIVE)
      {
        g_reload_CH446Q_setting = 1;
      }
      csh_printf(sh, "Change profile done\r\n");
    }

    return 0;
  }
  CSH_CMD_EXPORT(change_profile, );

  static int active_profile(int argc, char **argv)
  {
    chry_shell_t *sh = (chry_shell_t *)argv[argc + 1];
    if (
        (argc != 2) ||
        (strncmp(argv[1], "help", 4) == 0) ||
        (strcmp(argv[1], "--help") == 0) ||
        (strcmp(argv[1], "-h") == 0))
    {
      csh_printf(sh, "Active profile\r\n%s <ID>\r\n", argv[0]);
    }
    else
    {
      int id = atoi(argv[1]);
      extern storage_t g_storage;
      extern uint8_t g_reload_CH446Q_setting;
      if (id < 0 || id >= g_storage.num_of_profiles)
      {
        csh_printf(sh, "ID is not existed!\r\n");
        return 0;
      }
      for (int i = 0; i < g_storage.num_of_profiles; i++)
      {
        if (i == id)
        {
          g_storage.profiles[i].status = ACTIVE;
          g_reload_CH446Q_setting = 1;
        }
        else
        {
          g_storage.profiles[i].status = INACTIVE;
        }
      }
      storage_write(&g_storage);
      csh_printf(sh, "Active profile %d done\r\n", id);
    }

    return 0;
  }
  CSH_CMD_EXPORT(active_profile, );

  static int change_passwd(int argc, char **argv)
  {
    chry_shell_t *sh = (chry_shell_t *)argv[argc + 1];
    if (
        (strncmp(argv[1], "help", 4) == 0) ||
        (strcmp(argv[1], "--help") == 0) ||
        (strcmp(argv[1], "-h") == 0))
    {
      csh_printf(sh, "Change the password to login shell\r\n%s <PASSWORD_HASH>\r\n", argv[0]);
    }
    else
    {
      extern storage_t g_storage;
      if (strlen(argv[1]) >= MAX_STRING_LENGTH)
      {
        csh_printf(sh, "password hash is too long. Maximum size is %d\r\n", MAX_STRING_LENGTH);
        return -1;
      }
      strcpy(g_storage.shell_user.password, argv[1]);
      storage_write(&g_storage);
      csh_printf(sh, "New password has been set.\r\nPlease unplug and plug again to reload the new setting\r\n");
    }

    return 0;
  }
  CSH_CMD_EXPORT(change_passwd, );

  static int change_username(int argc, char **argv)
  {
    chry_shell_t *sh = (chry_shell_t *)argv[argc + 1];
    if (
        (argc < 2) ||
        (strncmp(argv[1], "help", 4) == 0) ||
        (strcmp(argv[1], "--help") == 0) ||
        (strcmp(argv[1], "-h") == 0))
    {
      csh_printf(sh, "Change the username\r\n%s <USERNAME>\r\n", argv[0]);
    }
    else
    {
      csh_printf(sh, "New username: %s\r\n", argv[1]);
      extern storage_t g_storage;
      if (strlen(argv[1]) >= MAX_STRING_LENGTH)
      {
        csh_printf(sh, "Username is too long. Maximum size is %d\r\n", MAX_STRING_LENGTH);
        return -1;
      }
      strcpy(g_storage.shell_user.username, argv[1]);
      storage_write(&g_storage);
      csh_printf(sh, "New username has been set.\r\nPlease unplug and plug again to reload the new setting\r\n");
    }

    return 0;
  }
  CSH_CMD_EXPORT(change_username, );

  static int change_hostname(int argc, char **argv)
  {
    chry_shell_t *sh = (chry_shell_t *)argv[argc + 1];
    if (
        (argc < 2) ||
        (strncmp(argv[1], "help", 4) == 0) ||
        (strcmp(argv[1], "--help") == 0) ||
        (strcmp(argv[1], "-h") == 0))
    {
      csh_printf(sh, "Change the hostname\r\n%s <HOSTNAME>\r\n", argv[0]);
    }
    else
    {
      csh_printf(sh, "Hostname: %s\r\n", argv[1]);
      extern storage_t g_storage;
      if (strlen(argv[1]) >= MAX_STRING_LENGTH)
      {
        csh_printf(sh, "Hostname is too long. Maximum size is %d\r\n", MAX_STRING_LENGTH);
        return -1;
      }
      strcpy(g_storage.shell_user.hostname, argv[1]);
      storage_write(&g_storage);
      csh_printf(sh, "New hostname has been set.\r\nPlease unplug and plug again to reload the new settings\r\n");
    }

    return 0;
  }
  CSH_CMD_EXPORT(change_hostname, );

  static int reset_factory(int argc, char **argv)
  {
    chry_shell_t *sh = (chry_shell_t *)argv[argc + 1];
    if ((strncmp(argv[1], "help", 4) == 0) ||
        (strcmp(argv[1], "--help") == 0) ||
        (strcmp(argv[1], "-h") == 0))
    {
      csh_printf(sh, "Factory reset the device\r\n");
    }
    else
    {
      storage_t storage;
      erase_storage();
      set_default_storage(&storage);
      storage_write(&storage);
      csh_printf(sh, "Reset factory done.\r\nPlease unplug and plug again to reload the new settings\r\n");
    }

    return 0;
  }
  CSH_CMD_EXPORT(reset_factory, );
  
static int reboot_dfu(int argc, char ** argv)
{
  chry_shell_t *sh = (chry_shell_t *)argv[argc + 1];
  if ((strncmp(argv[1], "help", 4) == 0) ||
      (strcmp(argv[1], "--help") == 0) ||
      (strcmp(argv[1], "-h") == 0))
  {
    csh_printf(sh, "Reboot the device to dfu mode\r\n");
  }
  else
  {
    csh_printf(sh, "Rebooting...\r\n");
    reset_usb_boot(0, 0);
  }

  return 0;

}
CSH_CMD_EXPORT(reboot_dfu, );
 
static int toggle_R120_resistor(int argc, char ** argv)
{
  chry_shell_t *sh = (chry_shell_t *) argv[argc+1];
  if ((strncmp(argv[1], "help", 4) == 0) ||
      (strcmp(argv[1], "--help") == 0) ||
      (strcmp(argv[1], "-h") == 0))
  {
    csh_printf(sh, "Enable/Disable internal R120 of CAN bus\r\n");
  }
  else
  {
    extern storage_t g_storage;
    if(g_storage.is_enable_120R)
    {
      csh_printf(sh, "Disabled R120 resistor\r\n");
      g_storage.is_enable_120R = 0;
    }else{
      csh_printf(sh, "Enabled R120 resistor\r\n");
      g_storage.is_enable_120R = 1;
    }
    storage_write(&g_storage);
  }

  return 0;
}
CSH_CMD_EXPORT_ALIAS(toggle_R120_resistor, toggle_resistor, );

static int help_OBDII_layout_male_connector(int argc, char ** argv)
{
  chry_shell_t *sh = (chry_shell_t *) argv[argc+1];
  /*
   *        No define ___________________     _______________________ No define          
   *        No define ________________   |   |   ____________________ CAN Low      
   * SAE J1850 Bus(-) _____________   |  |   |   |    _______________ K Line pin   
   *        No define ___________  |  |  |   |   |   |    ___________ Vehicle Battery Power
   *                             | |  |  |   |   |   |   |
   *                            ---------------------------
   *                           / 9 10 11 12  13  14 15  16 \
   *                          /                             \
   *                         /                               \
   *                        / 1   2   3   4   5   6   7   8   \
   *                        -----------------------------------
   *                          |   |   |   |   |   |   |   |
   *        No define --------    |   |   |   |   |   |    ---------- No define
   * SAE J1850 Bus(+) ------------    |   |   |   |    -------------- K line
   *       No define  ----------------    |   |    ------------------ CAN high
   *      Chassis GND --------------------     ---------------------- Signal Ground
   *    
   *    
   * */
  char layout[20][128] = {
    "--------------------------- OBD II Male Connector Layout ----------------------- ",
    "                                                                                 ",
    "        No define ___________________     _______________________ No define      ",
    "        No define ________________   |   |   ____________________ CAN Low        ",
    " SAE J1850 Bus(-) _____________   |  |   |   |    _______________ K Line pin     ",
    "        No define ___________  |  |  |   |   |   |    ___________ Vehicle Battery",
    "                             | |  |  |   |   |   |   |                           ",
    "                            ---------------------------                          ",
    "                           / 9 10 11 12  13  14 15  16 \\                        ",
    "                          /                             \\                       ",
    "                         /                               \\                      ",
    "                        / 1   2   3   4   5   6   7   8   \\                     ",
    "                        -----------------------------------                      ",
    "                          |   |   |   |   |   |   |   |                          ",
    "        No define --------    |   |   |   |   |   |    ---------- No define      ",
    " SAE J1850 Bus(+) ------------    |   |   |   |    -------------- K line         ",
    "       No define  ----------------    |   |    ------------------ CAN high       ",
    "      Chassis GND --------------------     ---------------------- Signal Ground  ",
    "                                                                                 ",
    "                                                                                 "
  };
  for(int i = 0; i < 20; i ++)
  {
    csh_printf(sh, "%s\r\n", layout[i]);  
  }
  
  return 0;
}
CSH_CMD_EXPORT_ALIAS(help_OBDII_layout_male_connector, OBDII_layout, );

// end shell command

#define __ENV_PATH "/sbin:/bin"
  const char ENV_PATH[] = __ENV_PATH;
  CSH_RVAR_EXPORT(ENV_PATH, PATH, sizeof(__ENV_PATH));

#define __ENV_ZERO ""
  const char ENV_ZERO[] = __ENV_ZERO;
  CSH_RVAR_EXPORT(ENV_ZERO, ZERO, sizeof(__ENV_ZERO));

#ifdef __cplusplus
}
#endif
