#include "ByteUtils.h"

uint16_t read_u16_le(const uint8_t *data) {
  return (uint16_t)(data[0] | (data[1] << 8));
}

uint32_t read_u32_le(const uint8_t *data) {
  return (uint32_t)data[0] | ((uint32_t)data[1] << 8) | ((uint32_t)data[2] << 16) |
         ((uint32_t)data[3] << 24);
}

uint16_t gen3_checksum32(const uint8_t *data, size_t length) {
  uint32_t chk = 0;
  for (size_t i = 0; i + 4 <= length; i += 4) {
    chk += read_u32_le(data + i);
  }
  return (uint16_t)(chk + (chk >> 16));
}
