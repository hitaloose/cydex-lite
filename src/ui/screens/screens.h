#pragma once

#include <lvgl.h>

// Cada função monta a tela inteira e devolve o objeto de tela (quem chama é
// nav_go). O que a tela aloca é liberado no LV_EVENT_DELETE dela.

lv_obj_t *boot_screen_create();      // L04 (canvas: Boot.dc.html)
lv_obj_t *sd_error_screen_create();  // L04b (ErroSD.dc.html)
lv_obj_t *showcase_screen_create();  // L01 (Main.dc.html)

enum class SdErrorReason : uint8_t { NoCard, NoData };
void sd_error_set_reason(SdErrorReason r);

// Rodapé do Claude da vitrine: redesenha só ele (chamado por ui/claude_ui
// quando claude_version() muda). Nada se a vitrine não estiver montada.
void showcase_update_footer();
