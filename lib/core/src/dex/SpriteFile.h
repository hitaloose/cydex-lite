#pragma once

#include <cstddef>
#include <cstdint>

// /cydex/sprites_<px>.bin: todos os sprites de um tamanho num arquivo só.
// FONTE DA VERDADE do formato (tools/build_sd.py grava igual).
//
// Por que um arquivo só: com 1025 arquivos numa pasta FAT32, cada abertura
// varre a pasta inteira (~560 ms por sprite medido na placa). Um arquivo com
// registros de tamanho fixo é aberto uma vez e cada sprite custa um seek.
//
// Cabeçalho (16 B):
//   0  char[4] "CYSP"
//   4  u16     versão (SPRITE_FILE_VERSION)
//   6  u16     quantidade de registros
//   8  u16     lado do sprite em px
//   10 u32     tamanho de cada registro em bytes
//   14 2 B     reservado
// Registro n (1..count) em SPRITE_HEADER_SIZE + (n-1) * registro:
//   lv_img_header_t (4 B, LV_IMG_CF_TRUE_COLOR_CHROMA_KEYED) + px*px RGB565 LE.
//   Espécie sem sprite: registro todo em chroma key (transparente); o flag
//   SPECIES_FLAG_HAS_SPRITE do species.bin diz se existe.

constexpr uint16_t SPRITE_FILE_VERSION = 1;
constexpr size_t SPRITE_HEADER_SIZE = 16;
constexpr size_t SPRITE_IMG_HEADER_SIZE = 4;

inline size_t sprite_record_size(uint16_t px) { return SPRITE_IMG_HEADER_SIZE + (size_t)px * px * 2; }

inline size_t sprite_record_offset(uint16_t px, uint16_t n) {
  return SPRITE_HEADER_SIZE + (size_t)(n - 1) * sprite_record_size(px);
}

struct SpriteFileHeader {
  uint16_t version;
  uint16_t count;
  uint16_t px;
  uint32_t record_size;
};

// Valida magic, versão e coerência do tamanho do registro.
bool sprite_header_decode(const uint8_t *data, SpriteFileHeader *out);
