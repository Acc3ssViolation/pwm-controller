#include "dcc/current_sense.h"
#include "sysb/events.h"
#include "arduino/mega.h"
#include "arduino/gpio.h"
#include <avr/interrupt.h>
#include <avr/cpufunc.h>

// The goal is to provide an averaged sample every millisecond or so. We use 32 samples
#define SAMPLE_SUM_SHIFT          (5)
#define MAX_SAMPLE_COUNT          (1 << SAMPLE_SUM_SHIFT)

static volatile uint16_t sampleCount;
static volatile uint32_t sampleSum;

void current_sense_initialize(void)
{
  // Use timer 1 for ADC clocking. We want 32 samples per millisecond.
  // Put it in fast PWM mode with OCRxA containing the TOP value. This allows us to control the ADC frequency using the timer overflow flag.
  TCCR1A = (1 << WGM11) | (1 << WGM10);
  TCCR1B = (1 << WGM12) | (1 << WGM13);
  // 16 MHz clock, with this we trigger at 32kHz
  OCR1A = 500;

  // Set up ADC. It takes 13 cycles for the ADC to perform its sampling.
  // Just put it at 1 MHz
  ADCSRA = (1 << ADIE) | ADC_PRESCALER_16;

  // Use ADC0 as input, this is pin PF0
  gpio_reset_pin(GPIO_PORT_F, GPIO_PIN_0);
  gpio_configure_input(GPIO_PORT_F, GPIO_PIN_0);
  ADMUX = (1 << REFS0) | ADC_CHANNEL_SINGLE_0;

  // Use timer 1 overflow as trigger source
  ADCSRB = ADC_TRIGGER_SOURCE_TIM1_OVF;
}

void current_sense_start(void)
{
  // Reset timer and state
  sampleCount = 0;
  sampleSum = 0;
  TCNT1 = 0;
  // Clear overflow flag
  TIFR1 |= (1 << TOV1);

  _MemoryBarrier();

  // Start timer
  TCCR1B |= TIMER_PRESCALER_1;

  // Enable ADC and auto trigger
  ADCSRA |= (1 << ADEN) | (1 << ADATE);
}

void current_sense_stop(void)
{
  // Disable ADC and auto trigger
  ADCSRA &= ~((1 << ADEN) | (1 << ADATE));

  // Stop timer
  TCCR1B &= ~(TIMER_PRESCALER_1);
}

ISR(ADC_vect)
{
  sampleSum += ADCW;
  if (++sampleCount == MAX_SAMPLE_COUNT)
  {
    message_t message;
    message.id = MESSAGE_ID_ADC_SAMPLES;

    sampleSum >>= SAMPLE_SUM_SHIFT;
    uint16_t *data = (uint16_t*)&message.data[0];
    *data = sampleSum;

    event_post_message(&message);

    sampleSum = 0;
    sampleCount = 0;
  }

  // Clear overflow flag
  TIFR1 |= (1 << TOV1);
}