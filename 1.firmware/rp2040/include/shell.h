/*
 * Copyright (c) 2022, Egahp
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SHELL_H
#define SHELL_H
#include <stdint.h>
#include "storage.h"

#ifdef __cplusplus
extern "C"
{
#endif
    extern int shell_init(storage_t *storage);
    extern int shell_main(void);
    extern void shell_uart_isr(char *data);
    extern void shell_lock(void);
    extern void shell_unlock(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
