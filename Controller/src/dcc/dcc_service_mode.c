#include "dcc/dcc_service_mode.h"
#include "dcc/dcc.h"
#include "sysb/timer.h"
#include "arduino/gpio.h"
#include <stddef.h>

#define DIRECT_MODE_MESSAGE_LENGTH    (4)
#define TIMEOUT_DURATION_MS           (500)
#define CURRENT_SENSE_HISTORY_LENGTH  (12)
#define ACKNOWLEDGE_PULSE_THRESHOLD   (5)

typedef enum
{
  SERVICE_MODE_STATE_IDLE,
  SERVICE_MODE_STATE_INIT_SEQUENCE,
  SERVICE_MODE_STATE_DIRECT_PACKETS,
  SERVICE_MODE_STATE_RECOVERY_TIME
} service_mode_state_t;

static const uint8_t m_resetMessage[] = { 0, 0, 0 };


static uint8_t m_directModeMessage[DIRECT_MODE_MESSAGE_LENGTH];
static uint8_t m_messageId;
static uint8_t m_timeoutTimer;
static uint8_t m_stateCounter;
static service_mode_state_t m_state;
static void* m_callbackUserData;
static dcc_callback_t m_callback;
static uint16_t m_currentSenseHistory[CURRENT_SENSE_HISTORY_LENGTH];
static bool m_ackDetected;

static void on_timer(uint8_t id);
static bool begin_direct_mode(uint16_t cvAddress, uint8_t firstByte, uint8_t data, dcc_callback_t callback, void* userData);
static void enter_state(service_mode_state_t state);
static void evaluate_state(void);

void dcc_service_mode_initialize(void)
{
  m_timeoutTimer = timer_create(TIMER_MODE_SINGLE, on_timer);
  m_state = SERVICE_MODE_STATE_IDLE;
}

bool dcc_service_mode_set_cv(uint16_t cvAddress, uint8_t data, dcc_callback_t callback, void* userData)
{
  // The fixed 0x7C indicates a direct mode write byte command
  return begin_direct_mode(cvAddress, 0x7C, data, callback, userData);
}

bool dcc_service_mode_verify_cv(uint16_t cvAddress, uint8_t data, dcc_callback_t callback, void* userData)
{
  // The fixed 0x7C indicates a direct mode verify byte command
  return begin_direct_mode(cvAddress, 0x74, data, callback, userData);
}

bool dcc_service_mode_set_cv_bit(uint16_t cvAddress, uint8_t bit, uint8_t data, dcc_callback_t callback, void* userData)
{
  // The fixed 0xF0 indicates a direct mode write bit command
  uint8_t dataByte = 0xF0 | (bit & 0x7) | ((data & 1) << 3);
  return begin_direct_mode(cvAddress, 0x78, dataByte, callback, userData);
}

bool dcc_service_mode_verify_cv_bit(uint16_t cvAddress, uint8_t bit, uint8_t data, dcc_callback_t callback, void* userData)
{
  // The fixed 0xE0 indicates a direct mode write bit command
  uint8_t dataByte = 0xE0 | (bit & 0x7) | ((data & 1) << 3);
  return begin_direct_mode(cvAddress, 0x78, dataByte, callback, userData);
}

void dcc_service_mode_tx_complete(const dcc_event_message_t* message)
{
  if (m_state == SERVICE_MODE_STATE_IDLE)
  {
    return;
  }

  if (message->dccMessageId == m_messageId)
  {
    // Message was transmitted, evaluate the state machine again
    evaluate_state();
  }
}

void dcc_service_mode_on_current_sense_data(uint16_t data)
{
  // Push back history
  for (uint8_t i = CURRENT_SENSE_HISTORY_LENGTH - 1; i > 0; --i)
  {
    m_currentSenseHistory[i] = m_currentSenseHistory[i - 1];
  }
  m_currentSenseHistory[0] = data;

  // Look for ACK pulses
  if ((m_state == SERVICE_MODE_STATE_DIRECT_PACKETS) || (m_state == SERVICE_MODE_STATE_RECOVERY_TIME))
  {
    // 0 1 2 3 4 5 6 7 8 9 10
    // L ? H H H H ? ? ? ? L
    // Check if index 0 and 10 are both low
    uint16_t lowLevel = (m_currentSenseHistory[0] + m_currentSenseHistory[10]) >> 1;
    uint16_t threshold = lowLevel + ACKNOWLEDGE_PULSE_THRESHOLD;
    if (
    (m_currentSenseHistory[2] > threshold) &&
    (m_currentSenseHistory[3] > threshold) &&
    (m_currentSenseHistory[4] > threshold) &&
    (m_currentSenseHistory[5] > threshold)
    )
    {
      // Found one!
      m_ackDetected = true;
      // TODO: We could stop sending more messages right now and going straight to the end
    }
  }


  uint8_t bits = data >> 7;
  

  gpio_set_pin_value(GPIO_PORT_E, GPIO_PIN_4, bits & 1);
  gpio_set_pin_value(GPIO_PORT_E, GPIO_PIN_5, bits & 2);
  gpio_set_pin_value(GPIO_PORT_G, GPIO_PIN_5, bits & 4);
}

static bool begin_direct_mode(uint16_t cvAddress, uint8_t firstByte, uint8_t data, dcc_callback_t callback, void* userData)
{
  if (m_state != SERVICE_MODE_STATE_IDLE)
  {
    // Already busy
    return false;
  }
  if (cvAddress > 0x3FF)
  {
    // CV address out of range
    return false;
  }

  // Set up configuration for state machine
  // First byte, some hardcoded bits and the top two bits of the address
  m_directModeMessage[0] = firstByte | (cvAddress >> 8);
  // Second byte, remaining address bits
  m_directModeMessage[1] = cvAddress & 0xFF;
  // Third byte, data
  m_directModeMessage[2] = data;
  // Fourth byte, error detection, XOR of all previous bytes
  m_directModeMessage[3] = m_directModeMessage[0] ^ m_directModeMessage[1] ^ m_directModeMessage[2];

  m_callbackUserData = userData;
  m_callback = callback;

  // Kick off state machine
  enter_state(SERVICE_MODE_STATE_INIT_SEQUENCE);
  evaluate_state();

  return true;
}

static void enter_state(service_mode_state_t state)
{
  switch (state)
  {
    case SERVICE_MODE_STATE_IDLE:
    {
      m_callback = NULL;
      m_stateCounter = 0;
      m_ackDetected = false;
      timer_stop(m_timeoutTimer);
      break;
    }
    case SERVICE_MODE_STATE_INIT_SEQUENCE:
    {
      m_stateCounter = 3;
      m_ackDetected = false;
      timer_start(m_timeoutTimer, TIMEOUT_DURATION_MS);
      break;
    }
    case SERVICE_MODE_STATE_DIRECT_PACKETS:
    {
      m_stateCounter = 5;
      m_ackDetected = false;
      timer_start(m_timeoutTimer, TIMEOUT_DURATION_MS);
      break;
    }
    case SERVICE_MODE_STATE_RECOVERY_TIME:
    {
      m_stateCounter = 6;
      // Note that we do not clear the ACK detection here, it may have been set properly during the DIRECT_PACKETS state
      timer_start(m_timeoutTimer, TIMEOUT_DURATION_MS);
      break;
    }
  }

  m_state = state;
}

static void evaluate_state(void)
{
  switch(m_state)
  {
    case SERVICE_MODE_STATE_IDLE:
    {
      // Idle is idle
      break;
    }
    case SERVICE_MODE_STATE_INIT_SEQUENCE:
    {
      if (m_stateCounter > 0)
      {
        dcc_queue_data_internal(&m_resetMessage[0], sizeof(m_resetMessage), &m_messageId);
        m_stateCounter--;
      }
      else
      {
        // Move to next state
        enter_state(SERVICE_MODE_STATE_DIRECT_PACKETS);
        // Kick off the first state evaluation
        evaluate_state();
      }
      break;
    }
    case SERVICE_MODE_STATE_DIRECT_PACKETS:
    {
      if (m_stateCounter > 0)
      {
        dcc_queue_data_internal(&m_directModeMessage[0], DIRECT_MODE_MESSAGE_LENGTH, &m_messageId);
        m_stateCounter--;
      }
      else
      {
        // Move to next state
        enter_state(SERVICE_MODE_STATE_RECOVERY_TIME);
        // Kick off the first state evaluation
        evaluate_state();
      }
      break;
    }
    case SERVICE_MODE_STATE_RECOVERY_TIME:
    {
      if (m_stateCounter > 0)
      {
        dcc_queue_data_internal(&m_resetMessage[0], sizeof(m_resetMessage), &m_messageId);
        m_stateCounter--;
      }
      else
      {
        // Finished
        if (m_callback != NULL)
        {
          dcc_result_t result = m_ackDetected ? DCC_RESULT_ACK : DCC_RESULT_NACK;
          m_callback(result, m_callbackUserData);
        }
        // Move to next state
        enter_state(SERVICE_MODE_STATE_IDLE);
      }
      break;
    }
  }
}

static void on_timer(uint8_t id)
{
  if (m_state == SERVICE_MODE_STATE_IDLE)
  {
    return;
  }

  // Timeout, abort
  if (m_callback != NULL)
  {
    m_callback(DCC_RESULT_TIMEOUT, m_callbackUserData);
  }

  enter_state(SERVICE_MODE_STATE_IDLE);
}