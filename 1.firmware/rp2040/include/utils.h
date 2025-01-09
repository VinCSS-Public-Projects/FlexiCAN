#ifndef __UTILS_H__
#define __UTILS_H__
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>

// #define delay_ms(x) for(int i=0; i<x*20000; i++) asm volatile("nop");
#define CYCLES_PER_US 125
#define delay_ms(ms) for (volatile int i = 0; i < (ms * 1000 * CYCLES_PER_US / 4); i++) asm volatile("nop");
#define delay_us(x) for (volatile int i = 0; i < (x * CYCLES_PER_US); i++) asm volatile("nop");

inline uint32_t *get_storage_addr() {
  extern uint32_t ADDR_PERSISTENT[];
  return ADDR_PERSISTENT;
}

char * str_to_upper(char * in);
int extract_pins(char * in, uint8_t * out);
int is_dupplicated_value(uint8_t * arr1, uint8_t* arr2, uint8_t size);
void array_to_string(char * out, size_t output_size,uint8_t *array, uint8_t size);
int is_all_zero(uint8_t *arr, uint8_t size);

#endif
