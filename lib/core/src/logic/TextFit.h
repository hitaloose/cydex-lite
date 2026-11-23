#pragma once

#include <cstddef>
#include <cstdint>

// Texto que vem de fora (avisos do Claude Code) cabendo na fonte e no espaço.

// A fonte do CYDex tem glifo para este código Unicode? Ela cobre
// 0x20-0x7E, 0xA0-0xFF e … ‹ › • – — “ ” − (tools/gen_fonts.mjs).
bool font_has_glyph(uint32_t cp);

// Copia `src` (UTF-8) para `dst` (`cap` bytes, com o '\0'):
//  - espaços, \n e \t viram um espaço só; as pontas são aparadas;
//  - ` * e # (marcação do markdown) somem;
//  - o que a fonte não desenha vira um equivalente (‘ ’ -> ', → -> "->"),
//    ou some (emoji, CJK);
//  - se não couber, corta numa fronteira de caractere e termina em "…".
// Retorna o tamanho escrito (sem o '\0').
size_t text_fit(const char *src, char *dst, size_t cap);
