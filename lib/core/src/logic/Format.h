#pragma once

#include <cstddef>
#include <cstdint>

// Valor em décimos com 1 casa e o separador do idioma: 7 -> "0,7", 1000 -> "100,0".
// Usado pra altura (dm -> m) e peso (hg -> kg) na Ficha e na Vitrine.
void format_tenths(uint16_t tenths, const char *sep, char *out, size_t size);

// Contador curto de tokens do rodapé do Claude: 950 -> "950", 31200 -> "31,2K",
// 1250000 -> "1,3M" (uma casa, separador do idioma; ",0" some: 2000 -> "2K").
void format_tokens(uint32_t n, const char *sep, char *out, size_t size);

// Hora local (epoch UTC + fuso em s) cai na noite [from_h, to_h)? A faixa pode
// virar a meia-noite (19 -> 7). Sem hora (epoch < 2024) = dia.
bool local_is_night(int64_t epoch, int32_t tz_s, uint8_t from_h, uint8_t to_h);
