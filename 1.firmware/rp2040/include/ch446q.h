#ifndef __CH446Q_H__
#define __CH446Q_H__


void CH446Q_init();
void CH446Q_reset();
void CH446Q_connect_pins(uint8_t * CAN_high_OBDII_pin, uint8_t * CAN_low_OBDII_pin);
void CH446q_refresh();
void CH446Q_switch_channel(uint8_t y, uint8_t x, uint8_t status);

#endif
