#include "arduino/vs1838b.h"
#include "arduino/mega.h"
#include "arduino/gpio.h"

#include "avr/interrupt.h"
#include "avr/io.h"
#include <stddef.h>

typedef enum
{
  RX_STATE_WAIT_FOR_START_BIT,
  RX_STATE_DATA,
} receive_state_t;

// Pin D0, INT0
static const gpio_info_t dataPin = GPIO_DIGITAL_21;

static receive_state_t rxState = RX_STATE_WAIT_FOR_START_BIT;
static vs1838b_rx_complete rxCallback = NULL;

#define MAX_RECEIVED_BITS     (64)
static volatile uint8_t receivedBits[MAX_RECEIVED_BITS];
static volatile uint8_t receivedBitCount;

#define TCCR1B_ENABLED_VALUE      ((1 << WGM12) | TIMER_PRESCALER_64)
#define TCCR1B_DISABLED_VALUE     (1 << WGM12)

#define TICKS_START_BIT_LOW       (4500 / 4)
#define TICKS_START_BIT_HIGH      (4500 / 4)

#define TICKS_ZERO_BIT_LOW        (560 / 4)
#define TICKS_ZERO_BIT_HIGH       (560 / 4)

#define TICKS_ONE_BIT_LOW         (560 / 4)
#define TICKS_ONE_BIT_HIGH        (1690 / 4)

void vs1838b_initialize(vs1838b_rx_complete callback)
{
  // Set up input pin with pull-up
  gpio_configure_input(dataPin.port, dataPin.pin);
  gpio_set_pin(dataPin.port, dataPin.pin);

  // Set up Timer 1 for our bit timing uses
  // CTC mode, prescaler 64 (250kHz -> 4us per tick)
  OCR1A = 1500;   // 6 ms
  TCCR1A = 0;
  TCCR1B = TCCR1B_DISABLED_VALUE;
  TCCR1C = 0;

    
  // Configure rising edge interrupt (end of burst)
  EIMSK &= ~(1 << INT0);
  EICRA |= (1 << ISC01) |  (1 << ISC00);

  rxCallback = callback;
}

void vs1838b_enable(void)
{
  cli();

  // Reset data
  receivedBitCount = 0;
  rxState = RX_STATE_WAIT_FOR_START_BIT;

  // Clear and enable interrupts on INT0
  EIFR = (1 << INTF0);
  EIMSK |= (1 << INT0);
  // Clear and enable timer overflow interrupt
  TIFR1 = (1 << OCF1A);
  TIMSK1 |= (1 << OCIE1A);

  sei();
}

void vs1838b_disable(void)
{
  cli();

  // Disable interrupts on INT0 and timer expiration
  EIMSK &= ~(1 << INT0);
  TIMSK1 &= (1 << OCIE1A);

  sei();
}


ISR(INT0_vect)
{
  uint16_t ticks;

  // Measure time since last interrupt and reset timer
  TCCR1B = TCCR1B_DISABLED_VALUE;
  ticks = TCNT1 * 4;
  TCNT1 = 0;
  TCCR1B = TCCR1B_ENABLED_VALUE;

  if (RX_STATE_WAIT_FOR_START_BIT == rxState)
  {
    if (ticks < 5500 && ticks > 4000)
    {
      rxState = RX_STATE_DATA;
      receivedBitCount = 0;
    }
  }
  else if (RX_STATE_DATA == rxState)
  {
    if (ticks < 900)
    {
      // Invalid, too short
      rxState = RX_STATE_WAIT_FOR_START_BIT;
    }
    else if (ticks < 1500)
    {
      // 0 data bit
      if (receivedBitCount < MAX_RECEIVED_BITS)
      {
        receivedBits[receivedBitCount++] = 0;
      }
    }
    else if (ticks < 2500)
    {
      // 1 data bit
      if (receivedBitCount < MAX_RECEIVED_BITS)
      {
        receivedBits[receivedBitCount++] = 1;
      }
    }
    else
    {
      // Invalid, too long
      rxState = RX_STATE_WAIT_FOR_START_BIT;
    }
  }

  //EIFR = (1 << INTF0);
}

ISR(TIMER1_COMPA_vect)
{
  // Timer ran out, end of message
  // Stop timer
  TCCR1B = TCCR1B_DISABLED_VALUE;

  if (rxCallback != NULL && rxState == RX_STATE_DATA)
  {
    rxCallback(&receivedBits[0], receivedBitCount);
  }
}