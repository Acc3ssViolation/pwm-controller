#include "serial.h"
#include "platform.h"
#include "buffers.h"
#include "events.h"
#include "atomic.h"
#include <avr/interrupt.h>
#include <avr/io.h>
#include <string.h>

#define RX_BUFFER_SIZE      (32)
#define TX_BUFFER_SIZE      (256)

static uint8_t rxBufferStorage[RX_BUFFER_SIZE];
static circular_buffer_t rxBuffer;
static volatile bool rxHasOverflowed;

static uint8_t txBufferStorage[TX_BUFFER_SIZE];
static circular_buffer_t txBuffer;

void serial_initialize(void)
{
  circular_buffer_initialize(&rxBuffer, &rxBufferStorage[0], sizeof(rxBufferStorage));
  circular_buffer_initialize(&txBuffer, &txBufferStorage[0], sizeof(txBufferStorage));

  // 8 bit data, no parity, 1 stop bit, TX + RX (interrupt based)
  // We use 2x transfer rate to reduce the clock frequency error at 115.2k baud rates
  UCSR0A = (1 << U2X0);
  UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
  UBRR0 = 16;    // 115.2k
  UCSR0B = (1 << TXEN0) | (1 << RXEN0) | (1 << RXCIE0);
}

bool serial_read_has_overflowed(void)
{
  return rxHasOverflowed;
}

bool serial_send_byte(uint8_t data)
{
  bool result = false;
  bool shouldTriggerTx = txBuffer.count == 0;

  NO_IRQ_BLOCK(UCSR0B, UDRIE0)
  {
    if (shouldTriggerTx && (UCSR0A & (1 << UDRE0)))
    {
      // Should start a new transmission because there is none active
      // We need to ensure the interrupt is enabled.
      // To do this we override the value that NO_IRQ_BLOCK saved before entering this block, so it will enable the interrupt when the block exits
      __NO_IRQ_BLOCK_RESTORE_MASK = (1 << UDRE0);
      UDR0 = data;
      result = true;
    }
    else
    {
      result = circular_buffer_write_byte(&txBuffer, data);
    }
  }

  return result;
}

bool serial_send(const uint8_t *data, uint8_t length)
{
  bool allWritten = false;

  if (length == 0) {
    return true;
  }

  NO_IRQ_BLOCK(UCSR0B, UDRIE0)
  {
    bool shouldTriggerTx = txBuffer.count == 0;

    if (shouldTriggerTx && (UCSR0A & (1 << UDRE0)))
    {
      // Should start a new transmission because there is none active
      allWritten = circular_buffer_write(&txBuffer, &data[1], length - 1);
      if (allWritten)
      {
        UDR0 = data[0];
        // We need to ensure the interrupt is enabled.
        // To do this we override the value that NO_IRQ_BLOCK saved before entering this block, so it will enable the interrupt when the block exits
        __NO_IRQ_BLOCK_RESTORE_MASK = (1 << UDRE0);
      }
    }
    else
    {
      allWritten = circular_buffer_write(&txBuffer, data, length);
    }
  }

  return allWritten;
}

uint8_t serial_read(uint8_t *data, uint8_t maxLength)
{
  uint8_t numReadBytes = maxLength;

  if (rxBuffer.count < numReadBytes)
  {
    numReadBytes = rxBuffer.count;
  }

  if (numReadBytes > 0)
  {
    // Prevent the interrupt from writing to the buffer while we read from it
    NO_IRQ_BLOCK(UCSR0B, RXCIE0)
    {
      circular_buffer_read(&rxBuffer, data, numReadBytes);
      rxHasOverflowed = false;
    }
  }

  return numReadBytes;
}

bool serial_read_byte(uint8_t *data)
{
  bool result = false;

  if (rxBuffer.count > 0)
  {
    // Prevent the interrupt from writing to the buffer while we read from it
    NO_IRQ_BLOCK(UCSR0B, RXCIE0)
    {
      result = circular_buffer_read_byte(&rxBuffer, data);
      rxHasOverflowed = false;
    }
  }

  return result;
}

ISR(USART_UDRE_vect)
{
  uint8_t data;
  if (circular_buffer_read_byte(&txBuffer, &data))
  {
    UDR0 = data;
  }
  else
  {
    // Disable ourselves to prevent continuously jumping into this interrupt while no data is transmitted
    UCSR0B &= ~(1 << UDRIE0);
  }
}

ISR(USART_RX_vect)
{
  uint8_t data = UDR0;

  if (false == circular_buffer_write(&rxBuffer, &data, 1))
  {
    rxHasOverflowed = true;
  }
}