#pragma once

#include <cstdint>

// Catálogo nacional.
constexpr uint16_t DEX_COUNT = 1025;
constexpr uint8_t DEX_GENERATIONS = 9;
// Um bit por espécie (n − 1): o "saco" da vitrine aleatória.
constexpr uint16_t DEX_BITS_BYTES = (DEX_COUNT + 7) / 8;

// Primeiro número de cada geração (I..IX) + sentinela DEX_COUNT+1.
constexpr uint16_t DEX_GEN_START[DEX_GENERATIONS + 1] = {1,   152, 252, 387, 494,
                                                         650, 722, 810, 906, 1026};

// 1..9; 0 se `n` estiver fora de 1..DEX_COUNT.
uint8_t dex_generation_of(uint16_t n);

// "I".."IX"; "" fora de 1..9. Não é traduzido.
const char *dex_generation_roman(uint8_t g);
