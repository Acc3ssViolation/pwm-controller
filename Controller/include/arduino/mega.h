
#ifndef MEGA_H_
#define MEGA_H_

#include <stdint.h>
#include <stdbool.h>
#include "arduino/gpio.h"

#define F_CPU     (16000000UL)
#define TIMER_PRESCALER_DISABLED      (0)
#define TIMER_PRESCALER_1             (1)
#define TIMER_PRESCALER_8             (2)
#define TIMER_PRESCALER_64            (3)
#define TIMER_PRESCALER_256           (4)
#define TIMER_PRESCALER_1024          (5)

#define ADC_PRESCALER_2               (1)
#define ADC_PRESCALER_4               (2)
#define ADC_PRESCALER_8               (3)
#define ADC_PRESCALER_16              (4)
#define ADC_PRESCALER_32              (5)
#define ADC_PRESCALER_64              (6)
#define ADC_PRESCALER_128             (7)

#define ADC_CHANNEL_SINGLE_0          (0)
#define ADC_CHANNEL_SINGLE_1          (1)
#define ADC_CHANNEL_SINGLE_2          (2)
#define ADC_CHANNEL_SINGLE_3          (3)
#define ADC_CHANNEL_SINGLE_4          (4)
#define ADC_CHANNEL_SINGLE_5          (5)
#define ADC_CHANNEL_SINGLE_6          (6)
#define ADC_CHANNEL_SINGLE_7          (7)
// These channels have the high bit (the 32) set in MUX5, which is in ADCSRB instead of ADMUX
//#define ADC_CHANNEL_SINGLE_8          (32)
//#define ADC_CHANNEL_SINGLE_9          (33)
//#define ADC_CHANNEL_SINGLE_10         (34)
//#define ADC_CHANNEL_SINGLE_11         (35)
//#define ADC_CHANNEL_SINGLE_12         (36)
//#define ADC_CHANNEL_SINGLE_13         (37)
//#define ADC_CHANNEL_SINGLE_14         (38)
//#define ADC_CHANNEL_SINGLE_15         (39)

#define ADC_TRIGGER_SOURCE_FREE_RUN   (0)
#define ADC_TRIGGER_SOURCE_COMPARATOR (1)
#define ADC_TRIGGER_SOURCE_EXT0       (2)
#define ADC_TRIGGER_SOURCE_TIM0_CMA   (3)
#define ADC_TRIGGER_SOURCE_TIM0_OVF   (4)
#define ADC_TRIGGER_SOURCE_TIM1_CMB   (5)
#define ADC_TRIGGER_SOURCE_TIM1_OVF   (6)
#define ADC_TRIGGER_SOURCE_TIM1_CE    (7)

#define GPIO_DIGITAL_18 { GPIO_PORT_D, GPIO_PIN_3 }
#define GPIO_DIGITAL_19 { GPIO_PORT_D, GPIO_PIN_2 }
#define GPIO_DIGITAL_20 { GPIO_PORT_D, GPIO_PIN_1 }
#define GPIO_DIGITAL_21 { GPIO_PORT_D, GPIO_PIN_0 }

#define GPIO_DIGITAL_22 { GPIO_PORT_A, GPIO_PIN_0 }
#define GPIO_DIGITAL_23 { GPIO_PORT_A, GPIO_PIN_1 }
#define GPIO_DIGITAL_24 { GPIO_PORT_A, GPIO_PIN_2 }
#define GPIO_DIGITAL_25 { GPIO_PORT_A, GPIO_PIN_3 }
#define GPIO_DIGITAL_26 { GPIO_PORT_A, GPIO_PIN_4 }
#define GPIO_DIGITAL_27 { GPIO_PORT_A, GPIO_PIN_5 }
#define GPIO_DIGITAL_28 { GPIO_PORT_A, GPIO_PIN_6 }
#define GPIO_DIGITAL_29 { GPIO_PORT_A, GPIO_PIN_7 }

#define GPIO_DIGITAL_30 { GPIO_PORT_C, GPIO_PIN_7 }
#define GPIO_DIGITAL_31 { GPIO_PORT_C, GPIO_PIN_6 }
#define GPIO_DIGITAL_32 { GPIO_PORT_C, GPIO_PIN_5 }
#define GPIO_DIGITAL_33 { GPIO_PORT_C, GPIO_PIN_4 }
#define GPIO_DIGITAL_34 { GPIO_PORT_C, GPIO_PIN_3 }
#define GPIO_DIGITAL_35 { GPIO_PORT_C, GPIO_PIN_2 }
#define GPIO_DIGITAL_36 { GPIO_PORT_C, GPIO_PIN_1 }
#define GPIO_DIGITAL_37 { GPIO_PORT_C, GPIO_PIN_0 }

#define GPIO_DIGITAL_50 { GPIO_PORT_B, GPIO_PIN_3 }
#define GPIO_DIGITAL_51 { GPIO_PORT_B, GPIO_PIN_2 }
#define GPIO_DIGITAL_52 { GPIO_PORT_B, GPIO_PIN_1 }
#define GPIO_DIGITAL_53 { GPIO_PORT_B, GPIO_PIN_0 }

void arduino_board_init(void);

void arduino_enable_led(void);

void arduino_disable_led(void);

void arduino_toggle_led(void);

#endif /* MEGA_H_ */