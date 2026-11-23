#include "ui/claude_ui.h"

#include <cstdio>
#include <initializer_list>

#include "app/claude_link.h"
#include "ble/ble_link.h"
#include "i18n/i18n.h"
#include "ui/icons/icons.h"
#include "ui/screens/screens.h"
#include "ui/theme.h"
#include "ui/widgets.h"

namespace {

constexpr lv_coord_t kPanelX = 3, kPanelW = 314, kPanelH = 162, kPanelPad = 10;
constexpr lv_coord_t kPanelInnerW = kPanelW - 2 - 2 * kPanelPad;  // 292 (borda 1)

lv_obj_t *s_prompt;   // camada do pedido (nullptr = fechado)
lv_obj_t *s_pairing;  // camada do passkey
uint32_t s_seen_claude, s_seen_prompt, s_seen_ble;

void close_prompt() {
  if (s_prompt) lv_obj_del(s_prompt);
  s_prompt = nullptr;
}

void on_deny(lv_event_t *) { claude_decide(false); }
void on_approve(lv_event_t *) { claude_decide(true); }

void open_prompt() {
  close_prompt();
  const PromptGate &g = claude_prompt();
  const BuddyPrompt &p = g.prompt();

  // Fundo esmaecido que também engole o toque (a vitrine não troca atrás).
  s_prompt = ui_box(lv_layer_top());
  lv_obj_set_size(s_prompt, LV_HOR_RES, LV_VER_RES);
  lv_obj_set_style_bg_color(s_prompt, c(theme::BG), 0);
  lv_obj_set_style_bg_opa(s_prompt, LV_OPA_60, 0);
  lv_obj_add_flag(s_prompt, LV_OBJ_FLAG_CLICKABLE);

  lv_obj_t *panel = ui_box(s_prompt);
  lv_obj_set_size(panel, kPanelW, kPanelH);
  lv_obj_set_pos(panel, kPanelX, LV_VER_RES - 3 - kPanelH);
  lv_obj_set_style_radius(panel, 10, 0);
  lv_obj_set_style_border_width(panel, 1, 0);
  lv_obj_set_style_border_color(panel, c(theme::ACCENT), 0);
  lv_obj_set_style_bg_color(panel, c(theme::SURFACE), 0);
  lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, 0);
  lv_obj_set_style_pad_all(panel, kPanelPad, 0);
  lv_obj_set_style_pad_row(panel, 8, 0);
  lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_COLUMN);

  // Cabeçalho: escudo + "Permissão · Bash" e o "+n" dos outros pedidos.
  lv_obj_t *head = ui_box(panel);
  lv_obj_set_size(head, kPanelInnerW, 18);
  lv_obj_set_style_pad_column(head, 6, 0);
  lv_obj_set_flex_flow(head, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(head, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  ui_icon(head, &icon_shield_16, theme::ACCENT);
  char title[48];
  snprintf(title, sizeof(title), tr(STR_PERM_TITLE_FMT), p.tool[0] ? p.tool : "?");
  lv_obj_t *t = ui_label(head, &font_14_700, theme::ACCENT, title);
  lv_obj_set_flex_grow(t, 1);
  lv_label_set_long_mode(t, LV_LABEL_LONG_DOT);
  if (g.waiting() > 1) {
    char more[8];
    snprintf(more, sizeof(more), "+%u", (unsigned)(g.waiting() - 1));
    lv_obj_t *chip = ui_chip(head, more, theme::SURFACE_2, theme::MUTED, 6);
    lv_obj_set_height(chip, 18);
    lv_obj_set_style_radius(chip, 9, 0);
  }

  // O comando (hint) em até 3 linhas.
  lv_obj_t *box = ui_box(panel);
  lv_obj_set_size(box, kPanelInnerW, 60);
  lv_obj_set_style_radius(box, 6, 0);
  lv_obj_set_style_bg_color(box, c(theme::BG), 0);
  lv_obj_set_style_bg_opa(box, LV_OPA_COVER, 0);
  lv_obj_set_style_pad_hor(box, 8, 0);
  lv_obj_set_style_pad_ver(box, 6, 0);
  lv_obj_t *hint = ui_label(box, &font_12_600, theme::TEXT, p.hint);
  lv_obj_set_style_text_line_space(hint, 1, 0);
  ui_label_clamp(hint, kPanelInnerW - 16, 3);

  // Negar | Aprovar, metade cada.
  lv_obj_t *row = ui_box(panel);
  lv_obj_set_size(row, kPanelInnerW, 44);
  lv_obj_set_style_pad_column(row, 8, 0);
  lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
  lv_obj_t *deny = ui_button(row, &icon_close_16, tr(STR_PERM_DENY), theme::SURFACE_2, theme::TEXT);
  lv_obj_t *ok = ui_button(row, &icon_check_16, tr(STR_PERM_APPROVE), theme::ACCENT, theme::ON_ACCENT);
  for (lv_obj_t *b : {deny, ok}) {
    lv_obj_set_style_pad_hor(b, 0, 0);
    lv_obj_set_flex_grow(b, 1);
  }
  ui_on_click(deny, on_deny);
  ui_on_click(ok, on_approve);
}

void close_pairing() {
  if (s_pairing) lv_obj_del(s_pairing);
  s_pairing = nullptr;
}

void open_pairing(uint32_t passkey) {
  close_pairing();
  s_pairing = ui_box(lv_layer_top());
  lv_obj_set_size(s_pairing, LV_HOR_RES, LV_VER_RES);
  lv_obj_set_style_bg_color(s_pairing, c(theme::BG), 0);
  lv_obj_set_style_bg_opa(s_pairing, LV_OPA_COVER, 0);
  lv_obj_add_flag(s_pairing, LV_OBJ_FLAG_CLICKABLE);

  lv_obj_t *col = ui_box(s_pairing);
  lv_obj_set_size(col, LV_PCT(100), LV_PCT(100));
  lv_obj_set_style_pad_hor(col, 24, 0);
  lv_obj_set_style_pad_ver(col, 16, 0);
  lv_obj_set_style_pad_row(col, 10, 0);
  lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(col, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  ui_icon(col, &icon_bluetooth_32, theme::BLUETOOTH);
  ui_label(col, &font_16_700, theme::TEXT, tr(STR_PAIR_TITLE));

  lv_obj_t *box = ui_box(col);
  lv_obj_set_size(box, LV_SIZE_CONTENT, 52);
  lv_obj_set_style_pad_hor(box, 18, 0);
  lv_obj_set_style_radius(box, 8, 0);
  lv_obj_set_style_bg_color(box, c(theme::SURFACE), 0);
  lv_obj_set_style_bg_opa(box, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(box, 1, 0);
  lv_obj_set_style_border_color(box, c(theme::LINE), 0);
  char code[8];
  snprintf(code, sizeof(code), "%03u %03u", (unsigned)(passkey / 1000), (unsigned)(passkey % 1000));
  lv_obj_t *pk = ui_label(box, &font_28_700, theme::TEXT, code);
  lv_obj_set_style_text_letter_space(pk, 4, 0);
  lv_obj_center(pk);

  lv_obj_t *hint = ui_label(col, &font_12_500, theme::MUTED, tr(STR_PAIR_HINT));
  lv_obj_set_style_text_align(hint, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_line_space(hint, 2, 0);

  lv_obj_t *name = ui_label(s_pairing, &font_12_600, theme::MUTED, ble_link_name());
  lv_obj_align(name, LV_ALIGN_BOTTOM_MID, 0, -8);
}

void watch(lv_timer_t *) {
  uint32_t v = claude_version();
  if (v != s_seen_claude) {
    s_seen_claude = v;
    showcase_update_footer();
  }
  v = claude_prompt_version();
  if (v != s_seen_prompt) {
    s_seen_prompt = v;
    if (claude_prompt().open()) open_prompt();
    else close_prompt();
  }
  v = ble_link_version();
  if (v != s_seen_ble) {
    s_seen_ble = v;
    uint32_t pk = ble_link_passkey();
    if (pk && !ble_link_secure()) open_pairing(pk);
    else close_pairing();
  }
}

}  // namespace

void claude_ui_init() { lv_timer_create(watch, 100, nullptr); }
