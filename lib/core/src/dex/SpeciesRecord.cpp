#include "SpeciesRecord.h"

#include <cstring>

#include "utils/ByteUtils.h"

static void write_u16_le(uint8_t *p, uint16_t v) {
  p[0] = (uint8_t)(v & 0xFF);
  p[1] = (uint8_t)(v >> 8);
}

static bool type_ok(uint8_t t, bool allow_none) {
  if (allow_none && t == (uint8_t)SpeciesType::None) return true;
  return t < SPECIES_TYPE_COUNT;
}

uint16_t SpeciesRecord::stat_total() const {
  uint16_t t = 0;
  for (int i = 0; i < SPECIES_STAT_COUNT; i++) t += stats[i];
  return t;
}

bool species_header_decode(const uint8_t *d, SpeciesFileHeader *out) {
  if (memcmp(d, "CYDX", 4) != 0) return false;
  out->version = read_u16_le(d + 4);
  out->count = read_u16_le(d + 6);
  out->record_size = read_u16_le(d + 8);
  return out->version == SPECIES_FILE_VERSION && out->record_size == SPECIES_RECORD_SIZE &&
         out->count > 0;
}

void species_header_encode(uint16_t count, uint8_t *out) {
  memset(out, 0, SPECIES_HEADER_SIZE);
  memcpy(out, "CYDX", 4);
  write_u16_le(out + 4, SPECIES_FILE_VERSION);
  write_u16_le(out + 6, count);
  write_u16_le(out + 8, SPECIES_RECORD_SIZE);
}

bool species_record_decode(const uint8_t *d, uint16_t number, SpeciesRecord *out) {
  memset(out, 0, sizeof(*out));
  out->number = number;

  if (memchr(d, 0, SPECIES_NAME_SIZE) == nullptr) return false;
  memcpy(out->name, d, SPECIES_NAME_SIZE);

  if (!type_ok(d[16], false) || !type_ok(d[17], true)) return false;
  out->type1 = (SpeciesType)d[16];
  out->type2 = (SpeciesType)d[17];
  out->height_dm = read_u16_le(d + 18);
  out->weight_hg = read_u16_le(d + 20);
  out->generation = d[22];
  memcpy(out->stats, d + 23, SPECIES_STAT_COUNT);

  out->evo_count = d[29];
  if (out->evo_count > SPECIES_EVO_MAX) return false;
  for (int i = 0; i < SPECIES_EVO_MAX; i++) out->evo[i] = read_u16_le(d + 30 + i * 2);
  out->evo_level[0] = d[36];
  out->evo_level[1] = d[37];
  out->flags = d[38];
  out->growth = d[39] <= 6 ? d[39] : 0;
  return true;
}

void species_record_encode(const SpeciesRecord &r, uint8_t *out) {
  memset(out, 0, SPECIES_RECORD_SIZE);
  strncpy((char *)out, r.name, SPECIES_NAME_SIZE - 1);
  out[16] = (uint8_t)r.type1;
  out[17] = (uint8_t)r.type2;
  write_u16_le(out + 18, r.height_dm);
  write_u16_le(out + 20, r.weight_hg);
  out[22] = r.generation;
  memcpy(out + 23, r.stats, SPECIES_STAT_COUNT);
  out[29] = r.evo_count;
  for (int i = 0; i < SPECIES_EVO_MAX; i++) write_u16_le(out + 30 + i * 2, r.evo[i]);
  out[36] = r.evo_level[0];
  out[37] = r.evo_level[1];
  out[38] = r.flags;
  out[39] = r.growth;
}
