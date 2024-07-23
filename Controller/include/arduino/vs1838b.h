#ifndef VS1838B_H_
#define VS1838B_H_

#include <stdint.h>
#include <stdbool.h>

typedef void (*vs1838b_rx_complete)(volatile uint8_t* bits, uint8_t nrOfBits);

void vs1838b_initialize(vs1838b_rx_complete callback);

void vs1838b_enable(void);

void vs1838b_disable(void);

#endif /* VS1838B_H_ */