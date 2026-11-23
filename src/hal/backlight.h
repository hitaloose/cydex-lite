#pragma once

#include <cstdint>

void backlight_init();
// 0..100 (%). 0 apaga a tela.
void backlight_set(uint8_t percent);
uint8_t backlight_get();
