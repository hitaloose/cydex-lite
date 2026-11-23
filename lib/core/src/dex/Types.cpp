#include "Types.h"

#include "i18n/i18n.h"

// Cores dos chips (tokens do CLAUDE.md). O nome vem do i18n.
static const uint32_t kColors[SPECIES_TYPE_COUNT] = {
    0xC9C2B4, 0xFF8A4C, 0x5AA8FF, 0x6FCF6A, 0xF5D547, 0x8FE0E6, 0xE8836B, 0xC78BE8, 0xE0B970,
    0xA8B8FF, 0xFF7FB0, 0xB5D45A, 0xCBB77A, 0xA99CE0, 0x9C8CFF, 0xA89A8C, 0xB7C6D3, 0xFFB3D9,
};

const char *species_type_name(SpeciesType t) {
  int i = (int)t;
  return (i >= 0 && i < SPECIES_TYPE_COUNT) ? tr((StrId)(STR_TYPE_NORMAL + i)) : "";
}

const char *species_type_abbr(SpeciesType t) {
  int i = (int)t;
  return (i >= 0 && i < SPECIES_TYPE_COUNT) ? tr((StrId)(STR_TYPE_ABBR_NORMAL + i)) : "";
}

uint32_t species_type_color(SpeciesType t) {
  int i = (int)t;
  return (i >= 0 && i < SPECIES_TYPE_COUNT) ? kColors[i] : 0x4A5562;
}
