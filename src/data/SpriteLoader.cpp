#include "SpriteLoader.h"

#include <cstdlib>
#include <cstring>

#include "dex/SpriteFile.h"
#include "hal/sd.h"
#include "paths.h"
#include "util/log.h"

static const char *TAG = "sprite";
static constexpr uint16_t kPx = 96;
static constexpr size_t kDataSize = (size_t)kPx * kPx * 2;  // 18432 B

static uint8_t *s_buf = nullptr;
static bool s_checked = false;
static uint16_t s_count = 0;

void sprite_reserve() {
  if (s_buf) return;
  s_buf = (uint8_t *)malloc(kDataSize);
  if (!s_buf) LOG_E(TAG, "sem bloco de %u B pro sprite", (unsigned)kDataSize);
}

bool sprite_load(uint16_t n, Sprite *out) {
  memset(out, 0, sizeof(*out));
  if (!s_buf) return false;
  size_t file_size = 0;
  if (!sd_open_read(SPRITES_96_PATH, &file_size)) return false;
  if (!s_checked) {
    uint8_t h[SPRITE_HEADER_SIZE];
    SpriteFileHeader hdr;
    if (!sd_read_at(0, h, sizeof(h)) || !sprite_header_decode(h, &hdr) || hdr.px != kPx ||
        file_size < sprite_record_offset(kPx, hdr.count + 1)) {
      LOG_E(TAG, "%s: cabeçalho inválido ou arquivo truncado", SPRITES_96_PATH);
      return false;
    }
    s_count = hdr.count;
    s_checked = true;
  }
  if (n < 1 || n > s_count) return false;
  lv_img_header_t header;
  size_t off = sprite_record_offset(kPx, n);
  bool ok = sd_read_at(off, (uint8_t *)&header, sizeof(header)) &&
            sd_read_at(off + sizeof(header), s_buf, kDataSize);
  if (!ok || header.w != kPx || header.h != kPx ||
      header.cf != LV_IMG_CF_TRUE_COLOR_CHROMA_KEYED) {
    LOG_E(TAG, "registro #%u inválido", n);
    return false;
  }
  out->dsc.header = header;
  out->dsc.data_size = kDataSize;
  out->dsc.data = s_buf;
  out->loaded = true;
  return true;
}
