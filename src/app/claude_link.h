#pragma once

#include <cstdint>

#include "buddy/BuddyState.h"

// Liga o link BLE ao protocolo do Hardware Buddy: lê as linhas, responde os
// cmds (acks, status), guarda o estado que a tela mostra e manda a decisão
// do pedido de permissão. Roda no loop (mesma task do LVGL, sem mutex).

void claude_init();
void claude_tick();

ClaudeState claude_state();
const BuddySnapshot &claude_snapshot();
const TokenLedger &claude_ledger();
// Rodapé: muda quando estado, contadores ou nível mudam.
uint32_t claude_version();

const PromptGate &claude_prompt();
// Muda quando o pedido abre, muda ou fecha.
uint32_t claude_prompt_version();
// Aprovar ("once") ou negar ("deny") o pedido aberto.
void claude_decide(bool approve);

// Hora local do {"time":[...]} do desktop (epoch UTC e fuso). false = sem hora.
bool claude_clock(int64_t *epoch, int32_t *tz_s);
