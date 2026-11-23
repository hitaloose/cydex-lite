#include "Settings.h"

#include <Preferences.h>

#include "logic/Showcase.h"
#include "util/log.h"

#ifndef CYDEX_DEFAULT_LANG
#define CYDEX_DEFAULT_LANG "pt-BR"
#endif

static const char *TAG = "settings";
static const char *NS = "cydexl";
// Chaves da NVS: no máximo 15 caracteres.
static const char *KEY_LANG = "lang";
static const char *KEY_SC_INT = "sc_int";
static const char *KEY_SC_RAND = "sc_rand";
static const char *KEY_BRIGHT = "bright";
static const char *KEY_BRIGHT_N = "bright_n";
static const char *KEY_NIGHT_FROM = "night_from";
static const char *KEY_NIGHT_TO = "night_to";
static const char *KEY_SOUND = "sound";
static const char *KEY_NAME = "name";
static const char *KEY_OWNER = "owner";
static const char *KEY_APPR = "appr";
static const char *KEY_DENY = "deny";
static const char *KEY_TOK_LIFE = "tok_life";
static const char *KEY_TOK_LAST = "tok_last";

static AppSettings s_settings;
static AppStats s_stats;

static uint8_t clamp_pct(uint8_t v) { return v < 5 ? 5 : (v > 100 ? 100 : v); }
static uint8_t clamp_h(uint8_t v, uint8_t d) { return v < 24 ? v : d; }

void settings_init() {
  Preferences p;
  // Leitura-escrita: em só-leitura o begin() falha enquanto o namespace não existe.
  p.begin(NS, false);
  AppSettings d;
  AppSettings &s = s_settings;
  String code = p.isKey(KEY_LANG) ? p.getString(KEY_LANG) : String(CYDEX_DEFAULT_LANG);
  uint16_t iv = p.isKey(KEY_SC_INT) ? p.getUShort(KEY_SC_INT) : d.showcase_interval_s;
  s.showcase_interval_s =
      SHOWCASE_INTERVALS_S[option_index(SHOWCASE_INTERVALS_S, SHOWCASE_INTERVAL_COUNT, iv)];
  s.showcase_random = p.isKey(KEY_SC_RAND) ? p.getBool(KEY_SC_RAND) : d.showcase_random;
  s.brightness = clamp_pct(p.isKey(KEY_BRIGHT) ? p.getUChar(KEY_BRIGHT) : d.brightness);
  s.brightness_night =
      clamp_pct(p.isKey(KEY_BRIGHT_N) ? p.getUChar(KEY_BRIGHT_N) : d.brightness_night);
  s.night_from_h = clamp_h(p.isKey(KEY_NIGHT_FROM) ? p.getUChar(KEY_NIGHT_FROM) : d.night_from_h,
                           d.night_from_h);
  s.night_to_h =
      clamp_h(p.isKey(KEY_NIGHT_TO) ? p.getUChar(KEY_NIGHT_TO) : d.night_to_h, d.night_to_h);
  s.sound = p.isKey(KEY_SOUND) ? p.getBool(KEY_SOUND) : d.sound;
  if (p.isKey(KEY_NAME)) p.getString(KEY_NAME, s.name, sizeof(s.name));
  if (p.isKey(KEY_OWNER)) p.getString(KEY_OWNER, s.owner, sizeof(s.owner));

  AppStats &st = s_stats;
  st.approved = p.isKey(KEY_APPR) ? p.getULong(KEY_APPR) : 0;
  st.denied = p.isKey(KEY_DENY) ? p.getULong(KEY_DENY) : 0;
  st.tokens_lifetime = p.isKey(KEY_TOK_LIFE) ? p.getULong64(KEY_TOK_LIFE) : 0;
  st.tokens_last = p.isKey(KEY_TOK_LAST) ? p.getULong(KEY_TOK_LAST) : 0;
  p.end();

  i18n_set_lang(i18n_lang_from_code(code.c_str()));
  LOG_I(TAG, "idioma=%s vitrine=%us/%s brilho=%u%%/%u%% noite=%uh-%uh som=%u nome=\"%s\"",
        i18n_lang_code(i18n_lang()), s.showcase_interval_s, s.showcase_random ? "aleat" : "seq",
        s.brightness, s.brightness_night, s.night_from_h, s.night_to_h, s.sound, s.name);
  LOG_I(TAG, "aprovou=%u negou=%u tokens=%llu", (unsigned)st.approved, (unsigned)st.denied,
        (unsigned long long)st.tokens_lifetime);
}

AppSettings &settings() { return s_settings; }
AppStats &stats() { return s_stats; }

void settings_save_names() {
  Preferences p;
  p.begin(NS, false);
  p.putString(KEY_NAME, s_settings.name);
  p.putString(KEY_OWNER, s_settings.owner);
  p.end();
}

void settings_save_stats() {
  Preferences p;
  p.begin(NS, false);
  // put* só grava na flash se o valor mudou.
  p.putULong(KEY_APPR, s_stats.approved);
  p.putULong(KEY_DENY, s_stats.denied);
  p.putULong64(KEY_TOK_LIFE, s_stats.tokens_lifetime);
  p.putULong(KEY_TOK_LAST, s_stats.tokens_last);
  p.end();
}
