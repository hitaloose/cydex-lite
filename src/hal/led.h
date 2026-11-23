#pragma once

// LED RGB da placa (4/16/17, comum-anodo: HIGH apaga).
void led_init();
// Pisca em vermelho (pedido de permissão aberto) ou apaga. Sem travar o loop:
// led_update() no loop faz o pisca.
void led_attention(bool on);
void led_update();
