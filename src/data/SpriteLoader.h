#pragma once

#include <lvgl.h>

#include <cstdint>

// Sprite 96×96 da vitrine, lido de /cydex/sprites_96.bin (formato em
// dex/SpriteFile.h): lv_img_header_t + RGB565 com fundo chroma key 0x00FF00.
// Um buffer fixo de 18 KB, pedido no setup antes do NimBLE e do FATFS: a
// vitrine só mostra um por vez. O CYDex reservava 46 KB pra Grade e a Ficha.

struct Sprite {
  lv_img_dsc_t dsc;
  bool loaded;
};

void sprite_reserve();
// Lê no buffer fixo (o anterior deixa de valer: soltar o lv_img antes).
bool sprite_load(uint16_t n, Sprite *out);
