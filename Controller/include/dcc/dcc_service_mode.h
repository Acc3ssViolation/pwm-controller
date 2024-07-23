#ifndef DCC_SERVICE_MODE_H_
#define DCC_SERVICE_MODE_H_
#include "dcc/dcc.h"
#include <stdbool.h>
#include <stdint.h>

typedef enum
{
  DCC_RESULT_ACK,
  DCC_RESULT_NACK,
  DCC_RESULT_TIMEOUT
} dcc_result_t;

typedef void (*dcc_callback_t)(dcc_result_t result, void* userData);

void dcc_service_mode_initialize(void);

void dcc_service_mode_on_current_sense_data(uint16_t data);

void dcc_service_mode_tx_complete(const dcc_event_message_t* message);

bool dcc_service_mode_set_cv(uint16_t cvAddress, uint8_t data, dcc_callback_t callback, void* userData);

bool dcc_service_mode_verify_cv(uint16_t cvAddress, uint8_t data, dcc_callback_t callback, void* userData);

bool dcc_service_mode_set_cv_bit(uint16_t cvAddress, uint8_t bit, uint8_t data, dcc_callback_t callback, void* userData);

bool dcc_service_mode_verify_cv_bit(uint16_t cvAddress, uint8_t bit, uint8_t data, dcc_callback_t callback, void* userData);

#endif /* DCC_SERVICE_MODE_H_ */