#ifndef SERIAL1_H_
#define SERIAL1_H_

#include <stdint.h>
#include <stdbool.h>

void serial1_initialize(void);

bool serial1_send_byte(uint8_t data);
bool serial1_read_byte(uint8_t *data);

bool serial1_send(const uint8_t *data, uint8_t length);
uint8_t serial1_read(uint8_t *data, uint8_t maxLength);

bool serial1_read_has_overflowed(void);

#endif /* SERIAL_H_ */