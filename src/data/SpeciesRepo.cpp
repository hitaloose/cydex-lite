#include "SpeciesRepo.h"

#include "dex/Dex.h"
#include "hal/sd.h"
#include "paths.h"
#include "util/log.h"

static const char *TAG = "species";
static uint16_t s_count = 0;

bool species_repo_init() {
  s_count = 0;
  size_t size = 0;
  if (!sd_open_read(SPECIES_PATH, &size)) return false;
  uint8_t header[SPECIES_HEADER_SIZE];
  SpeciesFileHeader h{};
  if (!sd_read_at(0, header, sizeof(header)) || !species_header_decode(header, &h)) {
    LOG_E(TAG, "cabeçalho inválido em %s", SPECIES_PATH);
    sd_close_read();
    return false;
  }
  uint16_t count = h.count > DEX_COUNT ? DEX_COUNT : h.count;
  if (count == 0 || size < species_record_offset(count + 1)) {
    LOG_E(TAG, "arquivo truncado (%u bytes)", (unsigned)size);
    sd_close_read();
    return false;
  }
  s_count = count;
  LOG_I(TAG, "%u espécies", s_count);
  return true;
}

uint16_t species_repo_count() { return s_count; }

bool species_repo_get(uint16_t n, SpeciesRecord *out) {
  if (n < 1 || n > s_count) return false;
  if (!sd_open_read(SPECIES_PATH, nullptr)) return false;
  uint8_t buf[SPECIES_RECORD_SIZE];
  bool ok = sd_read_at(species_record_offset(n), buf, sizeof(buf));
  return ok && species_record_decode(buf, n, out);
}
