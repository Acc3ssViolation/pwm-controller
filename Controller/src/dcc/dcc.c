#include "dcc/dcc.h"
#include "dcc/current_sense.h"
#include "arduino/gpio.h"
#include "arduino/platform.h"
#include "sysb/timer.h"
#include "sysb/events.h"
#include <stddef.h>
#include <string.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/cpufunc.h>

#define IDLE_TIME_MS                (28)        // Time since the start of the last message before we queue an idle message. Maximum is 30ms according to spec.
#define MAX_BUFFER_SIZE             (32)        // Max size of a DCC message buffer
#define NR_OF_BUFFERS               (5)         // Amount of DCC message buffers, depth of the transmission queue
#define NORMAL_PREAMBLE_LENGTH      (16)        // Amount of 1 bits in a normal preamble
#define LONG_PREAMBLE_LENGTH        (22)        // Amount of 1 bits in a long preamble used for service mode
#define ONE_BIT_HALF_PERIOD_US      (58)        // Half period duration when outputting a 1 bit
#define ZERO_BIT_HALF_PERIOD_US     (100)       // Half period duration when outputting a 0 bit

typedef struct
{
  bool inUse;
  dcc_message_flags_t flags;
  uint8_t messageId;
  uint8_t size;
  uint8_t data[MAX_BUFFER_SIZE];
} buffer_t;

typedef enum
{
  DCC_STATE_IDLE,
  DCC_STATE_PREAMBLE,
  DCC_STATE_START_BIT,
  DCC_STATE_DATA,
  DCC_STATE_STOP_BIT
} dcc_state_t;

static uint8_t nextMessageId;
static uint8_t idleInsertTimer;
static bool isRunning;
static buffer_t buffers[NR_OF_BUFFERS];
static volatile buffer_t* volatile transmitBuffer;    // Buffer being transmitted right now. May be NULL

// Pending data, set to a 10 1100 111000 11110000 1111100000 111111000000 11111110000000
//static uint8_t pendingData[] = { 0xb3, 0x8f, 0x0f, 0x83, 0xf0, 0x3f, 0x80 };
static volatile uint8_t pendingDataIndex = 0;
static volatile uint8_t bitsLeftToTransmit = 0;
static volatile uint8_t currentData = 0;
static volatile dcc_state_t state = DCC_STATE_IDLE;

static dcc_mode_t activeMode;

static bool queue_data(dcc_message_flags_t flags, const uint8_t* data, uint8_t size, uint8_t *messageIdOut);
static void on_idle_timer(uint8_t timer);

void dcc_initialize(void)
{
  // Use timer 4 for signal generation. We want to be able to have both 100us and 58us half periods, so 200us and 116us total periods.
  // Set up output compare for both channels. B uses normal mode, C uses inverted mode. A is used for period timing.
  // We also use mode 15 (Fast PWM with TOP in OCRA4) for waveform generation, so 1111 on WGM(3210). This ensures we have double
  // buffering as well on both or channels and the total period duration.
  TCCR4A = (1 << WGM41) | (1 << WGM40);
  // Prescaler of 8 gives us that 0.5us resolution
  TCCR4B = (1 << WGM42) | (1 << WGM43);
  // Set up OCR and ICR for a '1 bit' output signal
  OCR4B = 58 * 2;
  OCR4C = 58 * 2;
  OCR4A = 58 * 2 * 2;

  // Set up IOs as output
  DDRH |= (1 << DDH3) | (1 << DDH4) | (1 << DDH5);

  
    gpio_configure_output(GPIO_PORT_E, GPIO_PIN_4);
    gpio_configure_output(GPIO_PORT_E, GPIO_PIN_5);
    gpio_configure_output(GPIO_PORT_G, GPIO_PIN_5);

  gpio_configure_output(GPIO_PORT_B, GPIO_PIN_3);

  // Timer for inserting idle packets
  idleInsertTimer = timer_create(TIMER_MODE_SINGLE, on_idle_timer);

  current_sense_initialize();
}

void dcc_start(dcc_mode_t mode)
{
  bitsLeftToTransmit = 0;
  pendingDataIndex = 0;
  currentData = 0;
  transmitBuffer = NULL;
  state = DCC_STATE_IDLE;
  activeMode = mode;

  current_sense_start();

  _MemoryBarrier();

  // Reset timer
  TCNT4 = 0;
  // Enable pin outputs now
  TCCR4A |= (1 << COM4B1) | (1 << COM4C1) | (1 << COM4C0);

  _MemoryBarrier();

  // Start the timer
  TCCR4B |= TIMER_PRESCALER_8;

  _MemoryBarrier();

  // Enable interrupt for processing
  TIMSK4 = (1 << TOIE4);

  isRunning = true;

  // Start idle timer to ensure idle packets are transmitted if nothing is queued
  timer_start(idleInsertTimer, IDLE_TIME_MS);
}

void dcc_stop(void)
{
  current_sense_stop();

  // Disable interrupt for processing
  TIMSK4 &= ~(1 << TOIE4);

  _MemoryBarrier();

  // Disable pin outputs
  TCCR4A &= ~((1 << COM4B1) | (1 << COM4C1) | (1 << COM4C0));
  // Stop the timer
  TCCR4B &= ~TIMER_PRESCALER_8;
    
  isRunning = false;

  // No need to send idle packets anymore
  timer_stop(idleInsertTimer);
}

bool dcc_is_started(void)
{
  return isRunning;
}

dcc_mode_t dcc_get_mode(void)
{
  return activeMode;
}

void dcc_on_tx_started(void)
{
  if (!isRunning)
  {
    return;
  }

  uint8_t nrOfBuffersInUse = 0;

  for (int i = 0; i < NR_OF_BUFFERS; i++)
  {
    if (buffers[i].inUse)
    {
      nrOfBuffersInUse++;
    }
  }

  // Check if there is anything queued besides the packet currently being transmitted
  if (nrOfBuffersInUse <= 1) {
    // Start idle timer to ensure idle packets are transmitted after this one
    timer_start(idleInsertTimer, IDLE_TIME_MS);
  }
}

void dcc_on_tx_completed(void)
{
  if (!isRunning)
  {
    return;
  }

  // TODO: Send notification?
}

bool dcc_queue_data(const uint8_t* data, uint8_t size, uint8_t *messageIdOut)
{
  dcc_message_flags_t flags = DCC_MESSAGE_FLAG_USER_PROVIDED;
  if (DCC_MODE_SERVICE == activeMode)
  {
    flags |= DCC_MESSAGE_FLAG_LONG_PREAMBLE;
  }
  return queue_data(flags, data, size, messageIdOut);
}

bool dcc_queue_data_internal(const uint8_t* data, uint8_t size, uint8_t *messageIdOut)
{
  dcc_message_flags_t flags = 0;
  if (DCC_MODE_SERVICE == activeMode)
  {
    flags |= DCC_MESSAGE_FLAG_LONG_PREAMBLE;
  }
  return queue_data(flags, data, size, messageIdOut);
}

static bool queue_data(dcc_message_flags_t flags, const uint8_t* data, uint8_t size, uint8_t *messageIdOut)
{
  if ((size > MAX_BUFFER_SIZE) || (!isRunning))
  {
    return false;
  }

  for (int i = 0; i < NR_OF_BUFFERS; i++)
  {
    if (false == buffers[i].inUse)
    {
      // Found an empty buffer, try to place data in it
      memcpy(&buffers[i].data[0], data, size);
      buffers[i].size = size;
      buffers[i].flags = flags;
      buffers[i].messageId = nextMessageId;

      // Make sure that the inUse flag is set last so the interrupt can safely read the buffer contents
      _MemoryBarrier();
      
      buffers[i].inUse = true;

      if (NULL != messageIdOut)
      {
        *messageIdOut = nextMessageId;
      }

      nextMessageId++;

      return true;
    }
  }

  return false;
}


static void on_idle_timer(uint8_t timer)
{
  if (!isRunning)
  {
    return;
  }

  if (activeMode == DCC_MODE_SERVICE)
  {
    // Service mode, send reset message
    const uint8_t idleMsg[] = { 0x00, 0x00, 0x00 };
  
    queue_data(DCC_MESSAGE_FLAG_NONE, &idleMsg[0], sizeof(idleMsg), NULL);
  }
  else
  {
    // Operation mode, send idle message
    const uint8_t idleMsg[] = { 0xFF, 0x00, 0xFF };
    
    queue_data(DCC_MESSAGE_FLAG_NONE, &idleMsg[0], sizeof(idleMsg), NULL);
  }
}

ISR(TIMER4_OVF_vect)
{
  // ISR state
  static uint8_t preambleCounter;

  // ISR variables
  volatile buffer_t* newBuffer;
  uint8_t outputBit;
  message_t message;
  dcc_event_message_t* msgData = (dcc_event_message_t*)&message.data[0];

  switch (state)
  {
    case DCC_STATE_IDLE:
    {
      // Look for a new buffer to transmit
      for (int i = 0; i < NR_OF_BUFFERS; i++)
      {
        newBuffer = &buffers[i];
        if (newBuffer->inUse)
        {
          // Found a buffer to transmit, move to PREAMBLE state
          transmitBuffer = newBuffer;

          // Indicate to application that a transmission has started
          message.id = MESSAGE_ID_DCC_TX_STARTED;
          msgData->flags = transmitBuffer->flags;
          msgData->dccMessageId = transmitBuffer->messageId;
          event_post_message(&message);

          preambleCounter = (transmitBuffer->flags & DCC_MESSAGE_FLAG_LONG_PREAMBLE) ? LONG_PREAMBLE_LENGTH : NORMAL_PREAMBLE_LENGTH;
          state = DCC_STATE_PREAMBLE;
          break;
        }
      }
      // Idle state is just transmitting 1
      outputBit = 1;
      break;
    }
    case DCC_STATE_PREAMBLE:
    {
      // Output the preamble bits
      outputBit = 1;
      preambleCounter--;
      if (preambleCounter == 0)
      {
        // Get ready to transmit data
        pendingDataIndex = 0;
        state = DCC_STATE_START_BIT;
      }
      break;
    }
    case DCC_STATE_START_BIT:
    {
      // Output a 0, then output data
      outputBit = 0;

      bitsLeftToTransmit = 8;
      currentData = transmitBuffer->data[pendingDataIndex];
      pendingDataIndex++;

      state = DCC_STATE_DATA;
      break;
    }
    case DCC_STATE_DATA:
    {
      // Output data byte bits, MSB first
      if (bitsLeftToTransmit == 1)
      {
        // Last bit of byte, check which state to move to next
        if (pendingDataIndex >= transmitBuffer->size)
        {
          // Done with transmitting after this bit
          state = DCC_STATE_STOP_BIT;
        }
        else
        {
          // Got another data byte coming up
          state = DCC_STATE_START_BIT;
        }
      }

      outputBit = (currentData & 0x80);
      bitsLeftToTransmit--;
      currentData <<= 1;

      break;      
    }
    case DCC_STATE_STOP_BIT:
    {
      // Output a one bit and mark buffer as done
      outputBit = 1;

      // Indicate to application that a transmission has completed
      message.id = MESSAGE_ID_DCC_TX_COMPLETED;
      msgData->flags = transmitBuffer->flags;
      msgData->dccMessageId = transmitBuffer->messageId;
      event_post_message(&message);

      transmitBuffer->inUse = false;
      transmitBuffer = NULL;

      state = DCC_STATE_IDLE;

      break;
    }
    default:
    {
      // Should never end up here
      state = DCC_STATE_IDLE;
      outputBit = 1;
      transmitBuffer = NULL;
      break;
    }
  }

  uint8_t duration = outputBit ? ONE_BIT_HALF_PERIOD_US * 2 : ZERO_BIT_HALF_PERIOD_US * 2;
  OCR4B = duration;
  OCR4C = duration;
  OCR4A = duration * 2;
}
