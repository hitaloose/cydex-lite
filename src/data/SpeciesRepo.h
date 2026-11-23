#pragma once

#include <cstdint>

#include "dex/SpeciesRecord.h"

// Catálogo em /cydex/species.bin. Nada fica em RAM além da contagem: a vitrine
// mostra uma espécie por vez e lê o registro (64 B, com o nome) na troca.

// Confere o cabeçalho e o tamanho do arquivo. false = sem dados no cartão.
bool species_repo_init();
uint16_t species_repo_count();
bool species_repo_get(uint16_t n, SpeciesRecord *out);
