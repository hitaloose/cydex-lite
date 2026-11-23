#include "Format.h"

#include <cstdio>

void format_tenths(uint16_t tenths, const char *sep, char *out, size_t size) {
  snprintf(out, size, "%u%s%u", (unsigned)(tenths / 10), sep, (unsigned)(tenths % 10));
}

void format_tokens(uint32_t n, const char *sep, char *out, size_t size) {
  if (n < 1000) {
    snprintf(out, size, "%u", (unsigned)n);
    return;
  }
  const char *unit = n < 1000000 ? "K" : "M";
  uint32_t div = n < 1000000 ? 100 : 100000;  // décimos de K ou de M
  uint32_t t = (n + div / 2) / div;
  if (t >= 10000 && unit[0] == 'K') {  // 999,95K arredonda pra 1,0M
    unit = "M";
    t = (n + 50000) / 100000;
  }
  if (t % 10 == 0) snprintf(out, size, "%u%s", (unsigned)(t / 10), unit);
  else snprintf(out, size, "%u%s%u%s", (unsigned)(t / 10), sep, (unsigned)(t % 10), unit);
}

bool local_is_night(int64_t epoch, int32_t tz_s, uint8_t from_h, uint8_t to_h) {
  if (epoch < 1704067200) return false;  // 2024-01-01: antes disso, sem hora
  int64_t local = epoch + tz_s;
  int h = (int)(((local % 86400) + 86400) % 86400 / 3600);
  if (from_h == to_h) return false;
  if (from_h < to_h) return h >= from_h && h < to_h;
  return h >= from_h || h < to_h;
}
