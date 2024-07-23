#ifndef SERVO_H_
#define SERVO_H_

#include <stdint.h>

void servo_initialize(void);

void servo_enable(void);

void servo_disable(void);

void servo_set_angle(uint8_t angle_deg);


#endif /* SERVO_H_ */