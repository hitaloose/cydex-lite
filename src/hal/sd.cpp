#include "sd.h"

#include <SD.h>
#include <fcntl.h>
#include <unistd.h>

#include <cstdio>
#include <cstring>

#include "board_pins.h"
#include "util/log.h"

static const char *TAG = "sd";
static bool s_present = false;
static File s_read;
static char s_read_path[160];  // /saves/<nome de até 128> (o laço do boot)
static size_t s_read_size = 0;
// Escrita por open()/write() POSIX, não por SD.open: o FIL já foi reservado na
// montagem (max_files), e o File do Arduino + o FILE* do stdio pediam ~2 KB a
// mais de heap. Com o Wi-Fi o maior bloco fica em ~4 KB, e sem esses 2 KB o
// driver descartava pacotes e o upload parava na primeira janela do TCP.
static int s_write_fd = -1;
static const char kMount[] = "/sd";

// O padrão do SD.begin é 4 MHz. 20 MHz é estável no CYD (VSPI dedicada).
static const uint32_t kSdFreqHz = 20000000;
// Arquivos abertos ao mesmo tempo, no máximo: o de sd_open_read (reaproveitado)
// + pasta e entrada do sd_list_files. O FATFS reserva ~4 KB POR arquivo num
// bloco contíguo só; o padrão (5) pede ~25 KB e, com o Wi-Fi, não cabe.
static const uint8_t kMaxFiles = 3;

bool sd_init() {
  sd_close_read();
  // Já montado (Sistema › Recarregar): não desmonta. Remontar com o Wi-Fi
  // ligado falha, porque o FATFS pede ~17 KB contíguos de novo.
  if (s_present && SD.cardType() != CARD_NONE) return true;
  SD.end();
  if (!SD.begin(PIN_SD_CS, SPI, kSdFreqHz, "/sd", kMaxFiles)) {
    LOG_E(TAG, "SD.begin falhou (sem cartão ou não é FAT32?)");
    s_present = false;
    return false;
  }
  if (SD.cardType() == CARD_NONE) {
    LOG_E(TAG, "nenhum cartão detectado");
    s_present = false;
    return false;
  }
  LOG_I(TAG, "ok — tipo=%d %lluMB", SD.cardType(), SD.cardSize() / (1024 * 1024));
  s_present = true;
  return true;
}

bool sd_is_present() { return s_present; }

void sd_list_files(const char *dir_path, const char *ext, SdListCallback cb, void *user) {
  if (!s_present) return;
  File dir = SD.open(dir_path);
  if (!dir || !dir.isDirectory()) {
    LOG_E(TAG, "%s não é diretório", dir_path);
    return;
  }
  size_t ext_len = strlen(ext);
  File e = dir.openNextFile();
  while (e) {
    if (!e.isDirectory()) {
      const char *name = e.name();
      size_t n = strlen(name);
      if (n >= ext_len && strcasecmp(name + n - ext_len, ext) == 0) {
        cb(name, e.size(), (uint32_t)e.getLastWrite(), user);
      }
    }
    e.close();
    e = dir.openNextFile();
  }
  dir.close();
}

bool sd_open_read(const char *path, size_t *out_size) {
  if (!s_present) return false;
  if (s_read && strcmp(s_read_path, path) == 0) {
    if (out_size) *out_size = s_read_size;
    return true;
  }
  sd_close_read();
  s_read = SD.open(path, FILE_READ);
  if (!s_read) {
    LOG_E(TAG, "falha ao abrir %s", path);
    return false;
  }
  strncpy(s_read_path, path, sizeof(s_read_path) - 1);
  s_read_path[sizeof(s_read_path) - 1] = '\0';
  s_read_size = s_read.size();
  if (out_size) *out_size = s_read_size;
  return true;
}

bool sd_read_at(size_t offset, uint8_t *out, size_t len) {
  if (!s_read) return false;
  if (!s_read.seek(offset)) return false;
  return s_read.read(out, len) == len;
}

void sd_close_read() {
  if (s_read) s_read.close();
  s_read_path[0] = '\0';
  s_read_size = 0;
}

bool sd_read_whole(const char *path, uint8_t *out, size_t len) {
  if (!s_present) return false;
  File f = SD.open(path, FILE_READ);
  if (!f) return false;
  bool ok = f.size() == len && f.read(out, len) == len;
  f.close();
  return ok;
}

bool sd_write_whole(const char *path, const uint8_t *data, size_t len) {
  if (!s_present) return false;
  File f = SD.open(path, FILE_WRITE);
  if (!f) {
    LOG_E(TAG, "falha ao gravar %s", path);
    return false;
  }
  bool ok = f.write(data, len) == len;
  f.close();
  return ok;
}

bool sd_open_write(const char *path) {
  if (!s_present) return false;
  sd_close_write();
  char full[sizeof(kMount) + 96];
  snprintf(full, sizeof(full), "%s%s", kMount, path);
  s_write_fd = ::open(full, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (s_write_fd < 0) {
    LOG_E(TAG, "falha ao criar %s", path);
    return false;
  }
  return true;
}

bool sd_write(const uint8_t *data, size_t len) {
  if (s_write_fd < 0) return false;
  return ::write(s_write_fd, data, len) == (ssize_t)len;
}

void sd_close_write() {
  if (s_write_fd >= 0) ::close(s_write_fd);
  s_write_fd = -1;
}

bool sd_is_writing() { return s_write_fd >= 0; }

bool sd_exists(const char *path) { return s_present && SD.exists(path); }

bool sd_remove(const char *path) {
  if (!s_present) return false;
  if (!SD.exists(path)) return true;
  return SD.remove(path);
}

bool sd_rename(const char *from, const char *to) {
  if (!s_present) return false;
  // O rename do FATFS falha se o destino existe.
  if (SD.exists(to) && !SD.remove(to)) return false;
  return SD.rename(from, to);
}

bool sd_mkdir(const char *path) {
  if (!s_present) return false;
  return SD.exists(path) || SD.mkdir(path);
}
