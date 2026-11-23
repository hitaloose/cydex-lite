#include <unity.h>

#include <cstring>
#include <vector>

#include "dex/Dex.h"
#include "dex/SpeciesRecord.h"
#include "dex/SpriteFile.h"
#include "logic/Format.h"
#include "logic/Showcase.h"
#include "utils/ByteUtils.h"

void setUp() {}
void tearDown() {}

void test_generation_boundaries() {
  TEST_ASSERT_EQUAL(0, dex_generation_of(0));
  TEST_ASSERT_EQUAL(1, dex_generation_of(1));
  TEST_ASSERT_EQUAL(1, dex_generation_of(151));
  TEST_ASSERT_EQUAL(2, dex_generation_of(152));
  TEST_ASSERT_EQUAL(3, dex_generation_of(386));
  TEST_ASSERT_EQUAL(4, dex_generation_of(387));
  TEST_ASSERT_EQUAL(8, dex_generation_of(905));
  TEST_ASSERT_EQUAL(9, dex_generation_of(906));
  TEST_ASSERT_EQUAL(9, dex_generation_of(1025));
  TEST_ASSERT_EQUAL(0, dex_generation_of(1026));
}

void test_species_record_roundtrip() {
  SpeciesRecord r{};
  strcpy(r.name, "Flabébé");
  r.type1 = SpeciesType::Fairy;
  r.type2 = SpeciesType::None;
  r.height_dm = 1;
  r.weight_hg = 1;
  r.generation = 6;
  uint8_t stats[6] = {44, 38, 39, 61, 79, 42};
  memcpy(r.stats, stats, 6);
  r.evo_count = 3;
  r.evo[0] = 669;
  r.evo[1] = 670;
  r.evo[2] = 671;
  r.evo_level[0] = 19;
  r.evo_level[1] = 0;
  r.flags = SPECIES_FLAG_HAS_SPRITE;

  uint8_t buf[SPECIES_RECORD_SIZE];
  species_record_encode(r, buf);
  SpeciesRecord d;
  TEST_ASSERT_TRUE(species_record_decode(buf, 669, &d));
  TEST_ASSERT_EQUAL_STRING("Flabébé", d.name);
  TEST_ASSERT_EQUAL(669, d.number);
  TEST_ASSERT_TRUE(d.type1 == SpeciesType::Fairy);
  TEST_ASSERT_TRUE(d.type2 == SpeciesType::None);
  TEST_ASSERT_EQUAL(303, d.stat_total());
  TEST_ASSERT_EQUAL(671, d.evo[2]);
  TEST_ASSERT_EQUAL(19, d.evo_level[0]);
  TEST_ASSERT_EQUAL(SPECIES_FLAG_HAS_SPRITE, d.flags);
}

void test_species_record_rejects_bad_type_and_unterminated_name() {
  uint8_t buf[SPECIES_RECORD_SIZE] = {0};
  strcpy((char *)buf, "Bulbasaur");
  buf[16] = 0xFF;  // tipo 1 não pode ser "nenhum"
  SpeciesRecord d;
  TEST_ASSERT_FALSE(species_record_decode(buf, 1, &d));
  buf[16] = 3;
  buf[17] = 7;
  TEST_ASSERT_TRUE(species_record_decode(buf, 1, &d));
  memset(buf, 'A', SPECIES_NAME_SIZE);  // sem NUL
  TEST_ASSERT_FALSE(species_record_decode(buf, 1, &d));
}

void test_species_header() {
  uint8_t h[SPECIES_HEADER_SIZE];
  species_header_encode(1025, h);
  SpeciesFileHeader out;
  TEST_ASSERT_TRUE(species_header_decode(h, &out));
  TEST_ASSERT_EQUAL(1025, out.count);
  TEST_ASSERT_EQUAL(16 + 64 * 24, species_record_offset(25));
  h[0] = 'X';
  TEST_ASSERT_FALSE(species_header_decode(h, &out));
}

void test_sprite_file_header_and_offsets() {
  uint8_t h[SPRITE_HEADER_SIZE] = {'C', 'Y', 'S', 'P', 1, 0, 0x01, 0x04, 48, 0};
  uint32_t rec = 4 + 48 * 48 * 2;  // 4612
  h[10] = rec & 0xFF;
  h[11] = (rec >> 8) & 0xFF;
  SpriteFileHeader out;
  TEST_ASSERT_TRUE(sprite_header_decode(h, &out));
  TEST_ASSERT_EQUAL(1025, out.count);
  TEST_ASSERT_EQUAL(48, out.px);
  TEST_ASSERT_EQUAL(16, sprite_record_offset(48, 1));
  TEST_ASSERT_EQUAL(16 + 4612 * 24, sprite_record_offset(48, 25));
  h[10] ^= 1;  // tamanho de registro incoerente
  TEST_ASSERT_FALSE(sprite_header_decode(h, &out));
}

void test_format_tenths() {
  char buf[12];
  format_tenths(7, ",", buf, sizeof(buf));
  TEST_ASSERT_EQUAL_STRING("0,7", buf);
  format_tenths(69, ".", buf, sizeof(buf));
  TEST_ASSERT_EQUAL_STRING("6.9", buf);
  format_tenths(9999, ",", buf, sizeof(buf));
  TEST_ASSERT_EQUAL_STRING("999,9", buf);
  format_tenths(0, ",", buf, sizeof(buf));
  TEST_ASSERT_EQUAL_STRING("0,0", buf);
}

void test_option_step() {
  TEST_ASSERT_EQUAL(30, option_step(SHOWCASE_INTERVALS_S, SHOWCASE_INTERVAL_COUNT, 60, -1));
  TEST_ASSERT_EQUAL(30, option_step(SHOWCASE_INTERVALS_S, SHOWCASE_INTERVAL_COUNT, 30, -1));  // ponta
  TEST_ASSERT_EQUAL(900, option_step(SHOWCASE_INTERVALS_S, SHOWCASE_INTERVAL_COUNT, 900, 1));
  TEST_ASSERT_EQUAL(120, option_step(SHOWCASE_INTERVALS_S, SHOWCASE_INTERVAL_COUNT, 60, 1));
}

void test_showcase_next() {
  TEST_ASSERT_EQUAL(0, showcase_next(-1, 5, false, 0));
  TEST_ASSERT_EQUAL(3, showcase_next(2, 5, false, 0));
  TEST_ASSERT_EQUAL(0, showcase_next(4, 5, false, 0));  // volta ao começo
  for (uint32_t r = 0; r < 50; r++) {
    uint16_t n = showcase_next(2, 5, true, r * 7919);
    TEST_ASSERT_TRUE(n < 5);
    TEST_ASSERT_NOT_EQUAL(2, n);  // aleatória nunca repete
  }
  TEST_ASSERT_EQUAL(0, showcase_next(0, 1, true, 123));
  TEST_ASSERT_EQUAL(0, showcase_next(-1, 0, false, 0));
}

void test_showcase_pick() {
  // 161 espécies espalhadas: uma rodada mostra todas, cada uma uma vez.
  uint16_t list[161];
  for (uint16_t i = 0; i < 161; i++) list[i] = 1 + i * 6;
  uint8_t shown[DEX_BITS_BYTES] = {};
  uint32_t rnd = 12345;
  auto next_rnd = [&]() { rnd = rnd * 1103515245u + 12345u; return rnd >> 8; };
  uint16_t last = 0;
  for (int round = 0; round < 3; round++) {
    uint8_t seen_pos[161] = {};
    for (int k = 0; k < 161; k++) {
      uint16_t p = showcase_pick(list, 161, shown, last, next_rnd());
      TEST_ASSERT_TRUE(p < 161);
      if (k == 0) TEST_ASSERT_NOT_EQUAL(last, list[p]);  // rodada nova não repete a última
      TEST_ASSERT_EQUAL(0, seen_pos[p]);
      seen_pos[p] = 1;
      last = list[p];
    }
  }
  // Uma só: sempre ela.
  uint8_t one[DEX_BITS_BYTES] = {};
  uint16_t solo[1] = {25};
  for (int k = 0; k < 3; k++) TEST_ASSERT_EQUAL(0, showcase_pick(solo, 1, one, 25, k));
  // Lista muda no meio da rodada: a nova entra, as já mostradas continuam fora.
  uint8_t bag[DEX_BITS_BYTES] = {};
  uint16_t a[3] = {1, 2, 3};
  uint16_t first = a[showcase_pick(a, 3, bag, 0, 0)];
  uint16_t b[4] = {1, 2, 3, 4};
  uint8_t got[5] = {};
  got[first] = 1;
  for (int k = 0; k < 3; k++) {
    uint16_t n = b[showcase_pick(b, 4, bag, first, k * 7)];
    TEST_ASSERT_EQUAL(0, got[n]);
    got[n] = 1;
  }
}

void test_showcase_shift() {
  int8_t dx, dy;
  showcase_shift(0, &dx, &dy);
  TEST_ASSERT_EQUAL(0, dx);
  showcase_shift(5, &dx, &dy);  // 5 % 4 = 1
  TEST_ASSERT_EQUAL(2, dx);
  TEST_ASSERT_EQUAL(1, dy);
}

void test_showcase_crop() {
  constexpr uint16_t K = 0x07E0;
  static uint16_t px[96 * 96];
  for (auto &v : px) v = K;
  uint8_t x0 = 0, x1 = 0, y0 = 0, y1 = 0;
  TEST_ASSERT_FALSE(showcase_opaque_box(px, 96, K, &x0, &x1, &y0, &y1));
  px[10 * 96 + 30] = 0x1234;  // assimétrico: colunas 30 e 71 em linhas diferentes
  px[80 * 96 + 71] = 0;
  TEST_ASSERT_TRUE(showcase_opaque_box(px, 96, K, &x0, &x1, &y0, &y1));
  TEST_ASSERT_EQUAL(30, x0);
  TEST_ASSERT_EQUAL(71, x1);
  TEST_ASSERT_EQUAL(10, y0);
  TEST_ASSERT_EQUAL(80, y1);
  px[50 * 96 + 0] = 0;
  px[51 * 96 + 95] = 0;
  TEST_ASSERT_TRUE(showcase_opaque_box(px, 96, K, &x0, &x1, &y0, &y1));
  TEST_ASSERT_EQUAL(0, x0);
  TEST_ASSERT_EQUAL(95, x1);
  TEST_ASSERT_EQUAL(10, y0);
  TEST_ASSERT_EQUAL(80, y1);

  constexpr uint8_t W = SHOWCASE_CROP;
  TEST_ASSERT_EQUAL(12, showcase_crop_origin(30, 65, 96, W, true));   // pequena: centrada (48 - 36)
  TEST_ASSERT_EQUAL(0, showcase_crop_origin(0, 40, 96, W, true));     // encostada em cima: presa em 0
  TEST_ASSERT_EQUAL(24, showcase_crop_origin(70, 95, 96, W, false));  // encostada embaixo: presa em 24
  TEST_ASSERT_EQUAL(18, showcase_crop_origin(10, 89, 96, W, true));   // mais alta: fim no pé (90 - 72)
  TEST_ASSERT_EQUAL(14, showcase_crop_origin(10, 89, 96, W, false));  // mais larga: centrada
  TEST_ASSERT_EQUAL(24, showcase_crop_origin(0, 95, 96, W, true));    // 96 inteiros: pé embaixo
  TEST_ASSERT_EQUAL(0, showcase_crop_origin(0, 95, 48, 96, true));    // janela maior que o eixo
}


void test_format_tokens() {
  char b[16];
  format_tokens(0, ",", b, sizeof(b));
  TEST_ASSERT_EQUAL_STRING("0", b);
  format_tokens(950, ",", b, sizeof(b));
  TEST_ASSERT_EQUAL_STRING("950", b);
  format_tokens(31200, ",", b, sizeof(b));
  TEST_ASSERT_EQUAL_STRING("31,2K", b);
  format_tokens(2000, ",", b, sizeof(b));
  TEST_ASSERT_EQUAL_STRING("2K", b);
  format_tokens(184502, ".", b, sizeof(b));
  TEST_ASSERT_EQUAL_STRING("184.5K", b);
  format_tokens(999960, ",", b, sizeof(b));
  TEST_ASSERT_EQUAL_STRING("1M", b);
  format_tokens(1250000, ",", b, sizeof(b));
  TEST_ASSERT_EQUAL_STRING("1,3M", b);
}

void test_local_is_night() {
  // 2026-11-23 22:00 UTC; Brasília (−3 h) = 19:00 local.
  const int64_t t = 1795471200;
  TEST_ASSERT_TRUE(local_is_night(t, -3 * 3600, 19, 7));
  TEST_ASSERT_FALSE(local_is_night(t - 60, -3 * 3600, 19, 7));       // 18:59
  TEST_ASSERT_TRUE(local_is_night(t + 11 * 3600, -3 * 3600, 19, 7));   // 06:00
  TEST_ASSERT_FALSE(local_is_night(t + 12 * 3600, -3 * 3600, 19, 7));  // 07:00
  TEST_ASSERT_FALSE(local_is_night(1000, 0, 19, 7));                   // sem hora
  TEST_ASSERT_TRUE(local_is_night(t, 0, 20, 23));                      // 22:00 UTC
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_generation_boundaries);
  RUN_TEST(test_species_record_roundtrip);
  RUN_TEST(test_species_record_rejects_bad_type_and_unterminated_name);
  RUN_TEST(test_species_header);
  RUN_TEST(test_sprite_file_header_and_offsets);
  RUN_TEST(test_format_tenths);
  RUN_TEST(test_option_step);
  RUN_TEST(test_showcase_next);
  RUN_TEST(test_showcase_pick);
  RUN_TEST(test_showcase_shift);
  RUN_TEST(test_showcase_crop);
  RUN_TEST(test_format_tokens);
  RUN_TEST(test_local_is_night);
  return UNITY_END();
}
