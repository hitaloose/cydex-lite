#pragma once

#include <cstddef>
#include <cstdint>

// Nordic UART Service do Hardware Buddy (REFERENCE.md do
// anthropics/claude-desktop-buddy), com NimBLE só periférico.
//
//   serviço 6e400001-…  RX 6e400002-… (desktop -> CYD, write)
//                       TX 6e400003-… (CYD -> desktop, notify)
//
// Nome anunciado "Claude-CYDex-XXXX" (o picker filtra por "Claude"; XXXX =
// fim do MAC BT). LE Secure Connections com bonding: o CYD é DisplayOnly e as
// características exigem link cifrado e autenticado, então o 1º acesso dispara
// o pareamento e o desktop pede o passkey de 6 dígitos que a tela L03 mostra.
// Reconexões usam a chave guardada (NVS do NimBLE), sem passkey.
//
// Os bytes chegam no callback do NimBLE (outra task) e vão pra um anel de
// 2 KB; ble_link_tick() (loop, mesma task do LVGL) monta as linhas.

typedef void (*BleLineHandler)(const char *line, size_t len);

void ble_link_init();
void ble_link_tick(BleLineHandler on_line);

const char *ble_link_name();
bool ble_link_connected();
bool ble_link_secure();
// Passkey do pareamento em andamento (0 = nenhum).
uint32_t ble_link_passkey();
// Muda a cada conexão, queda, pareamento ou fim de pareamento.
uint32_t ble_link_version();

// Manda uma linha (o '\n' é acrescentado), picada no MTU.
void ble_link_send_line(const char *line);
// Esquece os pareamentos ({"cmd":"unpair"}) e derruba a conexão.
void ble_link_unpair();
