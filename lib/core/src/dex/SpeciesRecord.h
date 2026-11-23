#pragma once

#include <cstddef>
#include <cstdint>

#include "Types.h"

// Formato do /cydex/species.bin. FONTE DA VERDADE: tools/build_sd.py
// tem que gravar exatamente isto.
//
// Cabeçalho (16 B):
//   0  char[4]  magic "CYDX"
//   4  u16 LE   versão do formato (SPECIES_FILE_VERSION)
//   6  u16 LE   quantidade de registros (1025)
//   8  u16 LE   tamanho do registro (64)
//   10 6 B      reservado (0)
//
// Registro n (1..count) em SPECIES_HEADER_SIZE + (n-1) * SPECIES_RECORD_SIZE:
//   0  char[16] nome UTF-8, terminado em NUL (máx. 15 bytes)
//   16 u8       tipo 1 (SpeciesType)
//   17 u8       tipo 2 (SpeciesType, 0xFF = nenhum)
//   18 u16 LE   altura em decímetros
//   20 u16 LE   peso em hectogramas
//   22 u8       geração (1..9)
//   23 u8[6]    status base: PS, Atq, Def, AtE, DfE, Vel
//   29 u8       quantos estágios na linha evolutiva (0..3)
//   30 u16[3]   número de cada estágio (0 = vazio)
//   36 u8[2]    nível entre estágio i e i+1 (0 = sem nível: pedra, troca...)
//   38 u8       flags (bit0 = tem sprite)
//   39 u8       curva de crescimento (GrowthRate em logic/MonStats.h; 0 = não
//               se sabe: SD gerado antes dela, sem mudar a versão)
//   40 24 B     reservado (0)

constexpr uint16_t SPECIES_FILE_VERSION = 1;
constexpr size_t SPECIES_HEADER_SIZE = 16;
constexpr size_t SPECIES_RECORD_SIZE = 64;
constexpr size_t SPECIES_NAME_SIZE = 16;
constexpr int SPECIES_STAT_COUNT = 6;
constexpr int SPECIES_EVO_MAX = 3;
constexpr uint8_t SPECIES_FLAG_HAS_SPRITE = 0x01;

struct SpeciesFileHeader {
  uint16_t version;
  uint16_t count;
  uint16_t record_size;
};

struct SpeciesRecord {
  uint16_t number;
  char name[SPECIES_NAME_SIZE];
  SpeciesType type1;
  SpeciesType type2;
  uint16_t height_dm;
  uint16_t weight_hg;
  uint8_t generation;
  uint8_t stats[SPECIES_STAT_COUNT];
  uint8_t evo_count;
  uint16_t evo[SPECIES_EVO_MAX];
  uint8_t evo_level[SPECIES_EVO_MAX - 1];
  uint8_t flags;
  uint8_t growth;  // GrowthRate (0 = desconhecida)

  uint16_t stat_total() const;
};

// Valida magic/versão/tamanho. `data` tem SPECIES_HEADER_SIZE bytes.
bool species_header_decode(const uint8_t *data, SpeciesFileHeader *out);

// Offset do registro `n` no arquivo.
inline size_t species_record_offset(uint16_t n) {
  return SPECIES_HEADER_SIZE + (size_t)(n - 1) * SPECIES_RECORD_SIZE;
}

// Decodifica SPECIES_RECORD_SIZE bytes. false se o registro estiver
// inconsistente (tipo fora da faixa, nome sem NUL etc.).
bool species_record_decode(const uint8_t *data, uint16_t number, SpeciesRecord *out);

// Codifica (usado nos testes; o arquivo real vem do script Python).
void species_record_encode(const SpeciesRecord &rec, uint8_t *out);
void species_header_encode(uint16_t count, uint8_t *out);
