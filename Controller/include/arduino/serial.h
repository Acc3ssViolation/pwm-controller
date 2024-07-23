#ifndef SERIAL_H_
#define SERIAL_H_

#include <stdint.h>
#include <stdbool.h>

void serial_initialize(void);

bool serial_send_byte(uint8_t data);
bool serial_read_byte(uint8_t *data);

bool serial_send(const uint8_t *data, uint8_t length);
uint8_t serial_read(uint8_t *data, uint8_t maxLength);

bool serial_read_has_overflowed(void);

#endif /* SERIAL_H_ */