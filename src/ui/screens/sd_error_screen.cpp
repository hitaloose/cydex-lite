// L04b Sem cartão SD (canvas: ErroSD.dc.html). "Tentar de novo" volta pro Boot.
#include "i18n/i18n.h"
#include "screens.h"
#include "ui/icons/icons.h"
#include "ui/nav.h"
#include "ui/theme.h"
#include "ui/widgets.h"

static SdErrorReason s_reason = SdErrorReason::NoCard;

void sd_error_set_reason(SdErrorReason r) { s_reason = r; }

static void on_retry(lv_event_t *) { nav_go(Screen::Boot); }

lv_obj_t *sd_error_screen_create() {
  lv_obj_t *scr = ui_screen();

  lv_obj_t *col = ui_box(scr);
  lv_obj_set_size(col, LV_PCT(100), LV_PCT(100));
  lv_obj_set_style_pad_hor(col, 24, 0);
  lv_obj_set_style_pad_ver(col, 16, 0);
  lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(col, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_row(col, 10, 0);

  ui_icon(col, &icon_sd_error_44, theme::ACCENT);

  bool no_card = s_reason == SdErrorReason::NoCard;
  lv_obj_t *h1 = ui_label(col, &font_16_700, theme::TEXT,
                          tr(no_card ? STR_SDERR_NO_CARD_TITLE : STR_SDERR_NO_DATA_TITLE));
  lv_obj_set_style_text_align(h1, LV_TEXT_ALIGN_CENTER, 0);

  lv_obj_t *p = ui_label(col, &font_12_500, theme::MUTED,
                         tr(no_card ? STR_SDERR_NO_CARD_BODY : STR_SDERR_NO_DATA_BODY));
  lv_obj_set_width(p, LV_PCT(100));
  lv_label_set_long_mode(p, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_align(p, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_line_space(p, 2, 0);  // line-height 16 px

  // LVGL 8 não tem margem: os 4 px extras acima do botão vêm de um invólucro.
  lv_obj_t *wrap = ui_box(col);
  lv_obj_set_size(wrap, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
  lv_obj_set_style_pad_top(wrap, 4, 0);
  lv_obj_t *btn = ui_button(wrap, &icon_refresh_18, tr(STR_SDERR_RETRY), theme::ACCENT,
                            theme::ON_ACCENT);
  lv_obj_set_style_pad_column(btn, 8, 0);
  ui_on_click(btn, on_retry);

  return scr;
}
