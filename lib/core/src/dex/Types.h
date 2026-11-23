#pragma once

#include <cstdint>

// Ordem = valor gravado no species.bin (tools/build_sd.py usa a mesma).
enum class SpeciesType : uint8_t {
  Normal, Fire, Water, Grass, Electric, Ice, Fighting, Poison, Ground,
  Flying, Psychic, Bug, Rock, Ghost, Dragon, Dark, Steel, Fairy,
  Count,
  None = 0xFF,
};

constexpr int SPECIES_TYPE_COUNT = (int)SpeciesType::Count;

// Nome no idioma atual (i18n) e cor do chip (0xRRGGBB, tokens do CLAUDE.md).
const char *species_type_name(SpeciesType t);
// Nome curto de 3 letras (chips da Vitrine).
const char *species_type_abbr(SpeciesType t);
uint32_t species_type_color(SpeciesType t);
