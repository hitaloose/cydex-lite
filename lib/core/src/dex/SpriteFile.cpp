#include "SpriteFile.h"

#include <cstring>

#include "utils/ByteUtils.h"

bool sprite_header_decode(const uint8_t *d, SpriteFileHeader *out) {
  if (memcmp(d, "CYSP", 4) != 0) return false;
  out->version = read_u16_le(d + 4);
  out->count = read_u16_le(d + 6);
  out->px = read_u16_le(d + 8);
  out->record_size = read_u32_le(d + 10);
  return out->version == SPRITE_FILE_VERSION && out->px > 0 && out->count > 0 &&
         out->record_size == sprite_record_size(out->px);
}
