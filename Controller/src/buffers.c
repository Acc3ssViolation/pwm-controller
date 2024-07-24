#include "buffers.h"
#include <string.h>

static inline uint16_t wrap(const circular_buffer_t* buffer, uint16_t index);

void circular_buffer_initialize(circular_buffer_t* buffer, uint8_t* data, uint16_t size)
{
  buffer->data = data;
  buffer->size = size;
  buffer->readIndex = 0;
  buffer->count = 0;
}

bool circular_buffer_write(circular_buffer_t* buffer, const uint8_t* data, uint16_t size)
{
  if (buffer->count + size > buffer->size)
  {
    return false;
  }

  // Find write pointer
  uint16_t writeIndex = wrap(buffer, buffer->readIndex + buffer->count);

  // Write until end of buffer
  uint16_t copyLength = size;
  uint16_t writeUntilEndOfBuffer = buffer->size - writeIndex;

  if (copyLength > writeUntilEndOfBuffer)
  {
    copyLength = writeUntilEndOfBuffer;
  }
  memcpy(&buffer->data[writeIndex], data, copyLength);
  uint16_t leftoverLength = size - copyLength;

  if (leftoverLength > 0)
  {
    // Copy remaining data to the start of the buffer
    memcpy(&buffer->data[0], &data[copyLength], leftoverLength);
  }
  
  // Update buffer administration
  buffer->count += size;
  return true;
}

bool circular_buffer_read(circular_buffer_t* buffer, uint8_t* data, uint16_t size)
{
  if (buffer->count < size)
  {
    return false;
  }

  // Copy data until the end of the buffer (or size, whichever comes first)
  uint16_t copyLength = size;
  uint16_t readUntilEndOfBuffer = buffer->size - buffer->readIndex;

  if (copyLength > readUntilEndOfBuffer)
  {
    copyLength = readUntilEndOfBuffer;
  }
  memcpy(data, &buffer->data[buffer->readIndex], copyLength);
  uint16_t leftoverLength = size - copyLength;

  if (leftoverLength > 0)
  {
    // Copy remaining data that is at the start of the buffer
    memcpy(&data[copyLength], &buffer->data[0], leftoverLength);
  }
  
  // Update buffer administration
  buffer->count -= size;
  buffer->readIndex = wrap(buffer, buffer->readIndex + size);

  return true;
}

bool circular_buffer_write_byte(circular_buffer_t* buffer, uint8_t data)
{
  if (buffer->count + 1 > buffer->size)
  {
    return false;
  }

  // Find write pointer
  uint16_t writeIndex = wrap(buffer, buffer->readIndex + buffer->count);

  buffer->data[writeIndex] = data;
  buffer->count++;

  return true;
}

bool circular_buffer_read_byte(circular_buffer_t* buffer, uint8_t* data)
{
  if (buffer->count < 1)
  {
    return false;
  }

  *data = buffer->data[buffer->readIndex];
  buffer->readIndex = wrap(buffer, buffer->readIndex + 1);
  buffer->count--;

  return true;
}


static inline uint16_t wrap(const circular_buffer_t* buffer, uint16_t index)
{
  return index % buffer->size;
}