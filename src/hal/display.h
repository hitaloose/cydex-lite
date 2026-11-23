#pragma once

#include <TFT_eSPI.h>
#include <lvgl.h>

extern TFT_eSPI tft;

// Liga o painel em paisagem (320x240) e limpa a tela.
void display_init();

// Barras de cor + texto, sem LVGL: confere rotação e ordem de cores.
void display_self_test();

// Registra o driver de display no LVGL (buffer parcial em DRAM interna).
void display_lvgl_init();

// Redesenha a tela atual e manda os pixels pela Serial (comando "shot"; quem
// monta o PNG é tools/cydex_screenshot.py). Bloqueia enquanto a serial escoa.
void display_capture_to_serial(uint16_t y0, uint16_t y1);
