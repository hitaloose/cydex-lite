#pragma once

#include <cstddef>
#include <cstdint>

// Leitura little-endian genérica + checksum do formato Gen3. Sem dependência
// de Arduino — usado tanto no firmware quanto no ambiente de teste nativo
// (pio test -e native_test).

uint16_t read_u16_le(const uint8_t *data);
uint32_t read_u32_le(const uint8_t *data);

// CheckSum32 do Gen3 (GBA): soma todas as palavras de 32 bits do bloco como
// little-endian, depois soma a metade alta na metade baixa uma única vez.
// `length` deve ser múltiplo de 4. Algoritmo confirmado no código-fonte do
// PKHeX (Checksums.CheckSum32).
uint16_t gen3_checksum32(const uint8_t *data, size_t length);
