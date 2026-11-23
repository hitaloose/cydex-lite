#include <unity.h>

#include <cstdio>
#include <cstring>

#include "dex/Types.h"
#include "i18n/i18n.h"
#include "logic/TextFit.h"

void setUp() { i18n_set_lang(LANG_PT_BR); }
void tearDown() {}

// Conta os %... de um formato printf (ignora %%).
static int count_specs(const char *s) {
  int n = 0;
  for (; *s; s++) {
    if (*s != '%') continue;
    if (s[1] == '%') {
      s++;
      continue;
    }
    n++;
  }
  return n;
}

void test_every_language_has_every_string() {
  char msg[96];
  for (int l = 0; l < LANG_COUNT; l++) {
    for (int k = 0; k < STR_COUNT; k++) {
      snprintf(msg, sizeof(msg), "idioma %s sem texto na chave %d", i18n_lang_code((LangId)l), k);
      const char *s = i18n_raw((LangId)l, (StrId)k);
      TEST_ASSERT_NOT_NULL_MESSAGE(s, msg);
      TEST_ASSERT_TRUE_MESSAGE(s[0] != '\0', msg);
    }
  }
}

void test_format_specifiers_match_reference() {
  char msg[96];
  for (int l = 1; l < LANG_COUNT; l++) {
    for (int k = 0; k < STR_COUNT; k++) {
      const char *ref = i18n_raw(LANG_PT_BR, (StrId)k);
      const char *s = i18n_raw((LangId)l, (StrId)k);
      if (!ref || !s) continue;
      snprintf(msg, sizeof(msg), "idioma %s, chave %d: %%... diferente do pt-BR",
               i18n_lang_code((LangId)l), k);
      TEST_ASSERT_EQUAL_MESSAGE(count_specs(ref), count_specs(s), msg);
    }
  }
}

void test_codes_and_switching() {
  TEST_ASSERT_EQUAL(LANG_EN, i18n_lang_from_code("en"));
  TEST_ASSERT_EQUAL(LANG_PT_BR, i18n_lang_from_code("pt-BR"));
  TEST_ASSERT_EQUAL(LANG_PT_BR, i18n_lang_from_code("xx"));
  TEST_ASSERT_EQUAL(LANG_PT_BR, i18n_lang_from_code(nullptr));

  TEST_ASSERT_EQUAL_STRING("Água", species_type_name(SpeciesType::Water));
  i18n_set_lang(LANG_EN);
  TEST_ASSERT_EQUAL_STRING("Water", species_type_name(SpeciesType::Water));
  TEST_ASSERT_EQUAL_STRING("Fairy", species_type_name(SpeciesType::Fairy));
}

void test_type_keys_follow_enum_order() {
  TEST_ASSERT_EQUAL(SPECIES_TYPE_COUNT, STR_TYPE_FAIRY - STR_TYPE_NORMAL + 1);
  TEST_ASSERT_EQUAL_STRING("Normal", species_type_name(SpeciesType::Normal));
  TEST_ASSERT_EQUAL_STRING("Fada", species_type_name(SpeciesType::Fairy));
  TEST_ASSERT_EQUAL(SPECIES_TYPE_COUNT, STR_TYPE_ABBR_FAIRY - STR_TYPE_ABBR_NORMAL + 1);
  TEST_ASSERT_EQUAL_STRING("ÁGU", species_type_abbr(SpeciesType::Water));
  TEST_ASSERT_EQUAL_STRING("FAD", species_type_abbr(SpeciesType::Fairy));
}

void test_only_latin1_glyphs_plus_extras() {
  // Mesma tabela de glifos que o texto dos avisos usa (logic/TextFit).
  for (int l = 0; l < LANG_COUNT; l++) {
    for (int k = 0; k < STR_COUNT; k++) {
      const unsigned char *p = (const unsigned char *)i18n_raw((LangId)l, (StrId)k);
      while (p && *p) {
        unsigned cp;
        if (*p < 0x80) {
          cp = *p++;
        } else if ((*p & 0xE0) == 0xC0) {
          cp = ((p[0] & 0x1F) << 6) | (p[1] & 0x3F);
          p += 2;
        } else {
          cp = ((p[0] & 0x0F) << 12) | ((p[1] & 0x3F) << 6) | (p[2] & 0x3F);
          p += 3;
        }
        bool ok = cp == '\n' || font_has_glyph(cp);  // '\n' = quebra de linha, não glifo
        char msg[80];
        snprintf(msg, sizeof(msg), "idioma %s, chave %d: U+%04X sem glifo na fonte",
                 i18n_lang_code((LangId)l), k, cp);
        TEST_ASSERT_TRUE_MESSAGE(ok, msg);
      }
    }
  }
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_every_language_has_every_string);
  RUN_TEST(test_format_specifiers_match_reference);
  RUN_TEST(test_codes_and_switching);
  RUN_TEST(test_type_keys_follow_enum_order);
  RUN_TEST(test_only_latin1_glyphs_plus_extras);
  return UNITY_END();
}
