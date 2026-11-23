// L04 Boot (canvas: Boot.dc.html). Logo + "Lite", barra, "Lendo cartão SD…",
// "Ligando o Bluetooth" e a versão. Confere o cartão e segue pra Vitrine.
#include <Arduino.h>

#include "data/SpeciesRepo.h"
#include "hal/sd.h"
#include "i18n/i18n.h"
#include "screens.h"
#include "ui/nav.h"
#include "ui/theme.h"
#include "ui/widgets.h"
#include "util/log.h"
#include "version.h"

static const char *TAG = "boot";

static void load(lv_timer_t *t) {
  lv_timer_del(t);
  if (!sd_init()) {
    sd_error_set_reason(SdErrorReason::NoCard);
    nav_go(Screen::SdError);
    return;
  }
  if (!species_repo_init()) {
    sd_error_set_reason(SdErrorReason::NoData);
    nav_go(Screen::SdError);
    return;
  }
  LOG_I(TAG, "pronto: heap=%u maior_bloco=%u", ESP.getFreeHeap(),
        heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
  nav_go(Screen::Showcase);
}

lv_obj_t *boot_screen_create() {
  lv_obj_t *scr = ui_screen();

  lv_obj_t *col = ui_box(scr);
  lv_obj_set_size(col, LV_PCT(100), LV_PCT(100));
  lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(col, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_row(col, 16, 0);

  lv_obj_t *brand = ui_box(col);
  lv_obj_set_size(brand, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
  lv_obj_set_flex_flow(brand, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(brand, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_column(brand, 10, 0);
  ui_logo(brand, 28);
  ui_label(brand, &font_28_700, theme::TEXT, "CYDex");
  ui_chip(brand, "Lite", theme::SURFACE_2, theme::MUTED, 7);

  lv_obj_t *prog = ui_box(col);
  lv_obj_set_size(prog, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
  lv_obj_set_flex_flow(prog, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(prog, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_row(prog, 6, 0);
  lv_obj_t *bar = ui_progress(prog, 180, 6);
  ui_progress_set(bar, 1, 2);
  ui_label(prog, &font_12_600, theme::TEXT, tr(STR_BOOT_READING_SD));
  ui_label(prog, &font_12_500, theme::MUTED, tr(STR_BOOT_STARTING_BLE));

  lv_obj_t *ver = ui_label(scr, &font_12_500, theme::MUTED, "v" CYDEX_LITE_VERSION);
  lv_obj_align(ver, LV_ALIGN_BOTTOM_MID, 0, -8);

  // Deixa a tela aparecer antes da leitura bloqueante do cartão.
  lv_timer_create(load, 30, nullptr);
  return scr;
}
