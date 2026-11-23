#include "Showcase.h"

const uint16_t SHOWCASE_INTERVALS_S[SHOWCASE_INTERVAL_COUNT] = {30, 60, 120, 300, 900};

uint8_t option_index(const uint16_t *steps, uint8_t count, uint16_t value) {
  for (uint8_t i = 0; i < count; i++) {
    if (steps[i] == value) return i;
  }
  return 0;
}

uint16_t option_step(const uint16_t *steps, uint8_t count, uint16_t value, int8_t dir) {
  int i = option_index(steps, count, value) + dir;
  if (i < 0) i = 0;
  if (i >= count) i = count - 1;
  return steps[i];
}

uint16_t showcase_next(int32_t current, uint16_t count, bool random, uint32_t rnd) {
  if (count <= 1) return 0;
  if (current < 0 || current >= count) return random ? (uint16_t)(rnd % count) : 0;
  if (!random) return (uint16_t)((current + 1) % count);
  // Pula de 1 a count-1 posições: nunca cai na mesma.
  return (uint16_t)((current + 1 + rnd % (count - 1)) % count);
}

namespace {
bool bit(const uint8_t *bits, uint16_t n) { return bits[(n - 1) / 8] & (1u << ((n - 1) % 8)); }
void set_bit(uint8_t *bits, uint16_t n, bool on) {
  uint8_t m = 1u << ((n - 1) % 8);
  if (on) bits[(n - 1) / 8] |= m;
  else bits[(n - 1) / 8] &= ~m;
}
}  // namespace

uint16_t showcase_pick(const uint16_t *list, uint16_t count, uint8_t *shown, uint16_t last,
                       uint32_t rnd) {
  if (count == 0) return 0;
  uint16_t left = 0;
  for (uint16_t i = 0; i < count; i++) left += !bit(shown, list[i]);
  bool skip_last = false;
  if (left == 0) {
    for (uint16_t i = 0; i < count; i++) set_bit(shown, list[i], false);
    left = count;
    // Rodada nova: a última da anterior não abre esta (se houver outra).
    for (uint16_t i = 0; i < count && count > 1; i++) {
      if (list[i] == last) { skip_last = true; left--; break; }
    }
  }
  uint16_t k = (uint16_t)(rnd % left);
  for (uint16_t i = 0; i < count; i++) {
    if (bit(shown, list[i]) || (skip_last && list[i] == last)) continue;
    if (k-- == 0) {
      set_bit(shown, list[i], true);
      return i;
    }
  }
  return 0;  // não chega aqui
}

void showcase_shift(uint32_t k, int8_t *dx, int8_t *dy) {
  static const int8_t kShifts[4][2] = {{0, 0}, {2, 1}, {1, 2}, {-1, 1}};
  *dx = kShifts[k % 4][0];
  *dy = kShifts[k % 4][1];
}

bool showcase_opaque_box(const uint16_t *px, uint16_t size, uint16_t key, uint8_t *x0,
                         uint8_t *x1, uint8_t *y0, uint8_t *y1) {
  int lo = size, hi = -1, top = -1, bottom = -1;
  for (uint16_t y = 0; y < size; y++) {
    const uint16_t *row = px + (uint32_t)y * size;
    int first = -1;
    for (int x = 0; x < size; x++) {
      if (row[x] != key) { first = x; break; }
    }
    if (first < 0) continue;
    if (top < 0) top = y;
    bottom = y;
    if (first < lo) lo = first;
    for (int x = size - 1; x > hi; x--) {
      if (row[x] != key) { hi = x; break; }
    }
  }
  if (top < 0) return false;
  *x0 = (uint8_t)lo;
  *x1 = (uint8_t)hi;
  *y0 = (uint8_t)top;
  *y1 = (uint8_t)bottom;
  return true;
}

uint8_t showcase_crop_origin(uint8_t lo, uint8_t hi, uint8_t size, uint8_t win, bool anchor_end) {
  if (win >= size) return 0;
  int span = hi - lo + 1;
  int o = (span > win && anchor_end) ? hi + 1 - win : (lo + hi + 1 - win) / 2;
  if (o < 0) o = 0;
  if (o > size - win) o = size - win;
  return (uint8_t)o;
}
