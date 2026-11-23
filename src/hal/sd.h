#pragma once

#include <cstddef>
#include <cstdint>

// Acesso ao microSD (VSPI). Regras herdadas do PokEsp:
// - Nunca abrir arquivo dentro do callback de sd_list_files: o acesso
//   aninhado com o diretório aberto trava a lib SD. Listar primeiro,
//   abrir depois.
// - Arquivos grandes são lidos em pedaços (sd_read_at): sem PSRAM o heap
//   fragmenta e não sobra bloco contíguo grande.

// Monta o cartão. Pode ser chamada de novo ("Tentar de novo"): desmonta antes.
bool sd_init();
bool sd_is_present();

// Chamado uma vez por arquivo de `dir` cujo nome termina em `ext`
// (sem diferenciar maiúsculas). `name` vem sem o diretório.
typedef void (*SdListCallback)(const char *name, size_t size, uint32_t mtime, void *user);
void sd_list_files(const char *dir, const char *ext, SdListCallback cb, void *user);

// Um arquivo de leitura aberto por vez. Abrir o MESMO caminho de novo
// reaproveita o arquivo já aberto (abrir é caro: varre a pasta no FAT).
// Abrir outro caminho fecha o anterior.
bool sd_open_read(const char *path, size_t *out_size);
bool sd_read_at(size_t offset, uint8_t *out, size_t len);
void sd_close_read();

// Lê/escreve um arquivo pequeno inteiro (cache de status etc.).
bool sd_read_whole(const char *path, uint8_t *out, size_t len);
bool sd_write_whole(const char *path, const uint8_t *data, size_t len);

// Um arquivo de escrita aberto por vez (upload de save pela API). Trunca.
// sd_write devolve false se gravou menos que `len` (cartão cheio).
bool sd_open_write(const char *path);
bool sd_write(const uint8_t *data, size_t len);
void sd_close_write();
bool sd_is_writing();
// rename substitui o destino se já existir. remove de arquivo ausente = true.
bool sd_rename(const char *from, const char *to);
bool sd_remove(const char *path);
bool sd_exists(const char *path);
bool sd_mkdir(const char *path);  // true se já existe
