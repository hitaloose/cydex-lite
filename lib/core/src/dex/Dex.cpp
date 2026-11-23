#include "Dex.h"

uint8_t dex_generation_of(uint16_t n) {
  if (n < 1 || n > DEX_COUNT) return 0;
  for (uint8_t g = 0; g < DEX_GENERATIONS; g++) {
    if (n < DEX_GEN_START[g + 1]) return g + 1;
  }
  return 0;
}

const char *dex_generation_roman(uint8_t g) {
  static const char *const kRoman[DEX_GENERATIONS] = {"I",  "II",  "III",  "IV", "V",
                                                      "VI", "VII", "VIII", "IX"};
  return (g >= 1 && g <= DEX_GENERATIONS) ? kRoman[g - 1] : "";
}
