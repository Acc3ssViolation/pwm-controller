#ifndef BUFFERS_H_
#define BUFFERS_H_

#include <stdbool.h>
#include <stdint.h>

typedef struct  
{
  uint8_t* data;
  uint16_t size;
  uint16_t readIndex;
  uint16_t count;
} circular_buffer_t;

void circular_buffer_initialize(circular_buffer_t* buffer, uint8_t* data, uint16_t size);

bool circular_buffer_write(circular_buffer_t* buffer, const uint8_t* data, uint16_t size);

bool circular_buffer_read(circular_buffer_t* buffer, uint8_t* data, uint16_t size);

bool circular_buffer_write_byte(circular_buffer_t* buffer, uint8_t data);

bool circular_buffer_read_byte(circular_buffer_t* buffer, uint8_t* data);

#endif /* BUFFERS_H_ */