#pragma once

#include <cstdint>

#include "BuddyProto.h"

// Estado do Claude que a tela mostra (rodapé da L01 e o pedido da L02),
// derivado dos heartbeats. Sem relógio próprio: quem chama passa millis().

enum class ClaudeState : uint8_t {
  Disconnected,  // sem link, ou 30 s sem heartbeat
  Idle,
  Running,  // running > 0
  Waiting,  // waiting > 0 (pedido de permissão)
};

constexpr uint32_t BUDDY_STALE_MS = 30000;  // REFERENCE.md: ~30 s sem snapshot = morto
constexpr uint32_t TOKENS_PER_LEVEL = 50000;

// Total de tokens de saída de sempre (o "nível"). O `tokens` do heartbeat
// zera quando o app do computador reinicia, então soma só o que cresceu:
// valor menor que o último = app reiniciado, conta tudo de novo a partir dele.
// `lifetime` e `last` vão pra NVS (quem chama grava quando o nível muda).
struct TokenLedger {
  uint64_t lifetime = 0;
  uint32_t last = 0;

  // true se o nível mudou.
  bool add(uint32_t tokens);
  uint32_t level() const { return (uint32_t)(lifetime / TOKENS_PER_LEVEL); }
  // Progresso até o próximo nível, 0..999 (por mil).
  uint16_t progress_permille() const {
    return (uint16_t)((lifetime % TOKENS_PER_LEVEL) * 1000 / TOKENS_PER_LEVEL);
  }
};

// Abre/fecha o pedido da L02 a partir dos heartbeats.
//  - id novo que ainda não foi respondido aqui -> Open
//  - mesmo id, waiting diferente -> Update ("+n")
//  - heartbeat sem prompt com o pedido aberto -> Close (decidido no computador)
//  - respondido no CYD: fecha e não reabre o mesmo id se o próximo heartbeat
//    ainda o trouxer (corrida com o computador)
enum class PromptEvent : uint8_t { None, Open, Update, Close };

class PromptGate {
 public:
  PromptEvent on_snapshot(const BuddySnapshot &s);
  // O usuário decidiu no CYD. Devolve o id (pra montar a resposta).
  const char *answer();
  // Link caiu: fecha sem responder.
  PromptEvent on_disconnect();

  bool open() const { return open_; }
  const BuddyPrompt &prompt() const { return prompt_; }
  uint16_t waiting() const { return waiting_; }

 private:
  bool open_ = false;
  BuddyPrompt prompt_ = {};
  uint16_t waiting_ = 0;
  char answered_[sizeof(BuddyPrompt::id)] = "";
};

class BuddyState {
 public:
  void on_link(bool connected, uint32_t now_ms);
  void on_snapshot(const BuddySnapshot &s, uint32_t now_ms);
  // Confere o prazo do heartbeat. true se o estado mudou.
  bool tick(uint32_t now_ms);

  ClaudeState state() const;
  const BuddySnapshot &snap() const { return snap_; }
  // Muda a cada alteração que a tela mostra (a tela compara e só redesenha se mudou).
  uint32_t version() const { return version_; }
  void bump() { version_++; }

 private:
  bool linked_ = false;
  bool fresh_ = false;  // heartbeat nos últimos 30 s
  uint32_t last_ms_ = 0;
  BuddySnapshot snap_;
  uint32_t version_ = 1;
};
