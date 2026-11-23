#pragma once

#include <cstdint>

void touch_init();
// Lê o toque já em coordenadas de tela. false = sem toque.
bool touch_read(int16_t *x, int16_t *y);
// Registra o touch como indev de ponteiro no LVGL.
void touch_lvgl_init();

// Filtro do toque antes do LVGL (ui/power): recebe se há dedo na tela e
// devolve se o LVGL deve ver o toque. Usado pra "o 1o toque só acende a tela".
typedef bool (*TouchFilter)(bool pressed);
void touch_set_filter(TouchFilter f);
// Chamado a cada toque novo que chega ao LVGL (som ao tocar).
typedef void (*TouchPressHook)();
void touch_set_press_hook(TouchPressHook h);
