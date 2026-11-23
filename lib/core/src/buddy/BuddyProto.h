#pragma once

#include <cstddef>
#include <cstdint>

// Protocolo do Hardware Buddy (anthropics/claude-desktop-buddy, REFERENCE.md):
// JSON UTF-8, um objeto por linha, sobre o Nordic UART. Aqui só o parse e as
// respostas, sem BLE nem Arduino (testado em test/test_buddy).

// Pedido de permissão do heartbeat ("prompt"). Textos já passaram por
// text_fit (cabem na fonte); o id fica cru, porque volta igual na resposta.
struct BuddyPrompt {
  char id[80];
  char tool[24];
  char hint[112];
};

// Heartbeat: chega quando algo muda e a cada 10 s.
struct BuddySnapshot {
  uint16_t total = 0;    // sessões abertas
  uint16_t running = 0;  // gerando agora
  uint16_t waiting = 0;  // presas num pedido de permissão
  char msg[64] = "";
  uint32_t tokens = 0;        // tokens de saída desde que o app do computador abriu
  uint32_t tokens_today = 0;  // desde a meia-noite local
  bool has_prompt = false;
  BuddyPrompt prompt = {};
};

enum class BuddyKind : uint8_t {
  Invalid,    // não é JSON (ou não é objeto)
  Snapshot,   // heartbeat
  Time,       // {"time":[epoch, tz_s]}
  Turn,       // {"evt":"turn",...}: ignorado (sem tela de transcript)
  CmdStatus,  // {"cmd":"status"}
  CmdName,    // {"cmd":"name","name":"..."}
  CmdOwner,   // {"cmd":"owner","name":"..."}
  CmdUnpair,  // {"cmd":"unpair"}
  CmdFolder,  // char_begin/file/chunk/file_end/char_end: sem ack (o desktop desiste)
  CmdOther,   // outro cmd: ack com ok:false
  Other,      // objeto sem nada que interesse
};

struct BuddyMsg {
  BuddyKind kind = BuddyKind::Invalid;
  BuddySnapshot snap;     // Snapshot
  int64_t epoch = 0;      // Time
  int32_t tz_s = 0;       // Time: deslocamento do fuso em segundos
  char text[32] = "";     // CmdName/CmdOwner (text_fit)
  char cmd[24] = "";      // nome do cmd recebido (CmdOther ecoa no ack)
};

// Lê uma linha. Sempre preenche out->kind.
void buddy_parse(const char *line, size_t len, BuddyMsg *out);

// Respostas (sem o '\n'). Devolvem o tamanho escrito (0 se não coube).
size_t buddy_ack(char *out, size_t cap, const char *cmd, bool ok, uint32_t n,
                 const char *error = nullptr);

struct BuddyStatusInfo {
  const char *name;
  bool secure;
  uint32_t uptime_s;
  uint32_t heap;
  uint32_t approved;
  uint32_t denied;
  uint32_t level;
};
size_t buddy_status_ack(char *out, size_t cap, const BuddyStatusInfo &s);

// {"cmd":"permission","id":"...","decision":"once"|"deny"}
size_t buddy_permission(char *out, size_t cap, const char *id, bool approve);
