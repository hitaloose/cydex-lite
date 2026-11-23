#pragma once

// Camadas do Claude por cima de qualquer tela (lv_layer_top()):
//   L02 Permissão: painel na metade de baixo, vitrine esmaecida atrás.
//   L03 Pareamento: o passkey de 6 dígitos em tela cheia.
// Um timer de 100 ms compara as versões de app/claude_link e ble/ble_link e
// só redesenha o que mudou (também o rodapé da vitrine).
void claude_ui_init();
