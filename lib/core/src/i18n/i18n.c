#include "i18n.h"

#include <string.h>

extern const char *const LANG_PT_BR_STRINGS[STR_COUNT];
extern const char *const LANG_EN_STRINGS[STR_COUNT];

typedef struct {
  const char *code;
  const char *name;
  const char *const *strings;
} LangInfo;

static const LangInfo kLangs[LANG_COUNT] = {
    [LANG_PT_BR] = {"pt-BR", "Português", LANG_PT_BR_STRINGS},
    [LANG_EN] = {"en", "English", LANG_EN_STRINGS},
};

static LangId s_lang = LANG_PT_BR;

const char *tr(StrId id) {
  if ((unsigned)id >= STR_COUNT) return "";
  const char *s = kLangs[s_lang].strings[id];
  if (!s) s = LANG_PT_BR_STRINGS[id];
  return s ? s : "";
}

void i18n_set_lang(LangId lang) {
  if ((unsigned)lang < LANG_COUNT) s_lang = lang;
}

LangId i18n_lang(void) { return s_lang; }

const char *i18n_lang_code(LangId lang) {
  return (unsigned)lang < LANG_COUNT ? kLangs[lang].code : "";
}

const char *i18n_lang_name(LangId lang) {
  return (unsigned)lang < LANG_COUNT ? kLangs[lang].name : "";
}

LangId i18n_lang_from_code(const char *code) {
  if (code) {
    for (int i = 0; i < LANG_COUNT; i++) {
      if (strcmp(kLangs[i].code, code) == 0) return (LangId)i;
    }
  }
  return LANG_PT_BR;
}

const char *i18n_raw(LangId lang, StrId id) {
  if ((unsigned)lang >= LANG_COUNT || (unsigned)id >= STR_COUNT) return NULL;
  return kLangs[lang].strings[id];
}
