#pragma once

#include <cstddef>

// Buffer do display (15 KB) num bloco de heap pedido logo no começo do setup,
// antes de o NimBLE e o FATFS picarem o heap (o maior bloco no boot é ~47 KB).
// Nunca é liberado. Chamar early_alloc_init() antes de qualquer early_alloc().
void early_alloc_init();
void *early_alloc(size_t size);
