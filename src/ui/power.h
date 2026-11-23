#pragma once

#include <cstdint>

// Brilho da vitrine (dia ou noite) e o som ao tocar. A tela nunca apaga: é uma
// vitrine. "Noite" = hora local (do {"time"} do Claude Desktop) entre
// night_from_h e night_to_h dos Ajustes; sem hora, vale o de dia.
void power_init();
void power_tick();
// Chegou hora nova do desktop: reavalia dia/noite na hora.
void power_clock_changed();
// Pedido de permissão: hoje a tela já está sempre acesa; fica pra quando houver "apagar".
void power_wake();
