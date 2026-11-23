#pragma once

#include <cstdint>

#include "i18n/i18n.h"

// Ajustes e contadores persistentes (NVS via Preferences, namespace "cydexl").
// Sem tela de Ajustes na v1: os valores são os padrões abaixo (ou o que já
// estiver na NVS). settings_save_* grava só quando algo muda.

struct AppSettings {
  uint16_t showcase_interval_s = 60;  // SHOWCASE_INTERVALS_S (logic/Showcase.h)
  bool showcase_random = true;
  uint8_t brightness = 35;        // 5..100 %, de dia
  uint8_t brightness_night = 15;  // 5..100 %, entre night_from e night_to (hora local)
  uint8_t night_from_h = 19;
  uint8_t night_to_h = 7;
  bool sound = true;  // clique ao tocar + chime do pedido de permissão
  // Vindos do Claude Desktop ({"cmd":"name"} / {"cmd":"owner"}). "" = padrão.
  char name[32] = "";
  char owner[32] = "";
};

// Estatísticas do status ack e o total de tokens do nível (buddy/BuddyState.h).
struct AppStats {
  uint32_t approved = 0;
  uint32_t denied = 0;
  uint64_t tokens_lifetime = 0;
  uint32_t tokens_last = 0;
};

void settings_init();
AppSettings &settings();
AppStats &stats();
void settings_save_names();
void settings_save_stats();
