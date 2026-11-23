#pragma once

#include <lvgl.h>

#include "ui/fonts/fonts.h"

// Tokens do CYDex (CLAUDE.md, "Tokens"); o Lite usa os mesmos.
namespace theme {
constexpr uint32_t BG = 0x0F1317;
constexpr uint32_t SURFACE = 0x1A2027;
constexpr uint32_t SURFACE_2 = 0x232B34;
constexpr uint32_t LINE = 0x2C3540;
constexpr uint32_t TEXT = 0xECEFF2;
constexpr uint32_t MUTED = 0x93A0AD;
constexpr uint32_t FAINT = 0x4A5562;
constexpr uint32_t ACCENT = 0xFF5A4E;
constexpr uint32_t ON_ACCENT = 0x0F1317;  // texto sobre accent e chips de tipo
// Estado do Claude no rodapé (L01) e o passkey (L03).
constexpr uint32_t RUNNING = 0x6FCF6A;  // = Grama
constexpr uint32_t WAITING = 0xF5D547;  // = Elétrico
constexpr uint32_t BLUETOOTH = 0x5AA8FF;  // = Água
}  // namespace theme

inline lv_color_t c(uint32_t hex) { return lv_color_hex(hex); }
