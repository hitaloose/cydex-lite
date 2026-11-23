#pragma once

#include <cstdint>

// Três telas, uma por vez (a anterior é apagada antes de a próxima ser
// montada: as telas guardam estado em estáticos). Troca sem animação.
// Pedido de permissão (L02) e pareamento (L03) são camadas em lv_layer_top(),
// por cima de qualquer tela (ui/claude_ui).
enum class Screen : uint8_t { Boot, SdError, Showcase };

void nav_go(Screen s);
Screen nav_current();
