// L01 Vitrine com o Claude (canvas CYDex Lite: Main.dc.html).
//   cartão 314x234 sem moldura (3 px da borda da tela, padding 3/5):
//     cabeçalho 22: "#001" (14 muted) + nome (16, 1 linha, "…") e os tipos
//       abreviados à direita
//     corpo: janela 148x148 (borda 2 na cor do tipo 1 a 50% sobre o fundo; com
//       2 tipos, esquerda/baixo no tipo 1 e topo/direita no tipo 2) com o
//       sprite 96 em 2x cortado em 144 · medidas, geração e 6 status
//     rodapé 24 (borda 1 + padding 4): estado do Claude à esquerda (ponto +
//       texto) e, com link, "Nv n", barra até o próximo nível, "31,2K hoje"
//   Sem visto/capturado: toda espécie aparece completa.
//   Tocar em qualquer lugar = próxima espécie (o timer recomeça).
// Aleatória sem repetir até mostrar todas (showcase_pick). Só redesenha a
// carta na troca; o rodapé redesenha sozinho quando o Claude muda. Cada troca
// desloca tudo 1–2 px (anti-retenção).
#include <esp_random.h>

#include <cstdio>
#include <cstring>

#include "app/claude_link.h"
#include "data/Settings.h"
#include "data/SpeciesRepo.h"
#include "data/SpriteLoader.h"
#include "dex/Dex.h"
#include "i18n/i18n.h"
#include "logic/Format.h"
#include "logic/Showcase.h"
#include "screens.h"
#include "ui/theme.h"
#include "ui/widgets.h"
#include "util/log.h"

namespace {

constexpr lv_coord_t kCardX = 3, kCardY = 3, kCardW = 314, kCardH = 234;
constexpr lv_coord_t kPadV = 3, kPadH = 5, kGap = 4;
constexpr lv_coord_t kInnerW = kCardW - 2 * kPadH;  // 304
constexpr lv_coord_t kHeadH = 22, kFootH = 24;
constexpr lv_coord_t kBodyY = kHeadH + kGap;
constexpr lv_coord_t kBodyH = kCardH - 2 * kPadV - kHeadH - kFootH - 2 * kGap;  // 174
constexpr lv_coord_t kWinW = 148, kWinBorder = 2;  // janela de 144 por dentro
constexpr lv_coord_t kWinY = kBodyY + (kBodyH - kWinW) / 2;
constexpr lv_coord_t kColX = kWinW + 8, kColW = kInnerW - kColX;  // 148
constexpr lv_coord_t kStatH = 14, kStatGap = 3, kStatLabelW = 26, kStatValueW = 24;
constexpr lv_coord_t kStatBarX = kStatLabelW + 4 + kStatValueW + 4, kStatBarH = 4;
constexpr lv_coord_t kChipPad = 7, kChipGap = 4;
constexpr lv_coord_t kLevelBarW = 52;
constexpr uint16_t kChromaKey = 0x07E0;  // lv_color_hex(0x00FF00) em RGB565 (build_sd.py)

uint16_t s_list[DEX_COUNT];  // 1..count (todas entram)
uint16_t s_count;
int32_t s_pos = -1;  // sobrevive entre sessões: a sequencial continua de onde parou
uint8_t s_shown[DEX_BITS_BYTES];  // saco da aleatória (só RAM)
uint16_t s_last_n;
uint32_t s_shift_k;
Sprite s_sprite;
lv_obj_t *s_card;
lv_obj_t *s_foot;
lv_timer_t *s_timer;
lv_obj_t *s_scr;

const StrId kStatLabels[SPECIES_STAT_COUNT] = {STR_STAT_HP,  STR_STAT_ATK, STR_STAT_DEF,
                                               STR_STAT_SPA, STR_STAT_SPD, STR_STAT_SPE};

lv_coord_t text_w(const char *text, const lv_font_t *font) {
  return lv_txt_get_width(text, strlen(text), font, 0, LV_TEXT_FLAG_NONE);
}

// Cabeçalho: tipos à direita, "#nº" à esquerda, o nome fica com o que sobra.
void build_header(uint16_t n, const SpeciesRecord *rec) {
  lv_obj_t *chips = ui_box(s_card);
  lv_obj_set_size(chips, LV_SIZE_CONTENT, 20);
  lv_obj_set_style_pad_column(chips, kChipGap, 0);
  lv_obj_set_flex_flow(chips, LV_FLEX_FLOW_ROW);
  lv_obj_align(chips, LV_ALIGN_TOP_RIGHT, 0, (kHeadH - 20) / 2);
  lv_coord_t used = 0;
  if (rec) {
    for (SpeciesType t : {rec->type1, rec->type2}) {
      if (t == SpeciesType::None) continue;
      const char *ab = species_type_abbr(t);
      ui_chip(chips, ab, species_type_color(t), theme::ON_ACCENT, kChipPad);
      used += (used ? kChipGap : 0) + text_w(ab, &font_12_700) + 2 * kChipPad;
    }
  }

  char num[8];
  snprintf(num, sizeof(num), tr(STR_SC_NUM_FMT), n);
  lv_obj_t *nl = ui_label(s_card, &font_14_700, theme::MUTED, num);
  lv_obj_align(nl, LV_ALIGN_TOP_LEFT, 0, (kHeadH - lv_font_get_line_height(&font_14_700)) / 2);
  lv_coord_t num_w = text_w(num, &font_14_700) + 6;

  lv_coord_t free_w = kInnerW - num_w - used - (used ? 8 : 0);
  lv_obj_t *name = ui_label(s_card, &font_16_700, theme::TEXT, rec ? rec->name : "");
  ui_label_clamp(name, free_w, 1);
  lv_obj_align(name, LV_ALIGN_TOP_LEFT, num_w,
               (kHeadH - lv_font_get_line_height(&font_16_700)) / 2);
}

// Janela 148x148: borda na cor do tipo 1 (desenhada por cima do sprite) e o
// sprite 96 em 2x preso no canto, com a origem do corte em (ox, oy).
void build_window(uint16_t n, const SpeciesRecord *rec) {
  lv_color_t tint = rec ? lv_color_mix(c(species_type_color(rec->type1)), c(theme::BG), LV_OPA_50)
                        : c(theme::FAINT);
  lv_obj_t *win = ui_box(s_card);
  lv_obj_set_size(win, kWinW, kWinW);
  lv_obj_set_pos(win, 0, kWinY);
  lv_obj_set_style_radius(win, 8, 0);
  lv_obj_set_style_clip_corner(win, true, 0);
  lv_obj_set_style_bg_color(win, c(theme::BG), 0);
  lv_obj_set_style_bg_opa(win, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(win, kWinBorder, 0);
  lv_obj_set_style_border_color(win, tint, 0);
  lv_obj_set_style_border_post(win, true, 0);

  if (!sprite_load(n, &s_sprite)) return;
  const uint16_t side = s_sprite.dsc.header.w;
  uint8_t x0 = 0, x1 = side - 1, y0 = 0, y1 = side - 1;
  showcase_opaque_box((const uint16_t *)s_sprite.dsc.data, side, kChromaKey, &x0, &x1, &y0, &y1);
  uint8_t ox = showcase_crop_origin(x0, x1, side, SHOWCASE_CROP, false);
  uint8_t oy = showcase_crop_origin(y0, y1, side, SHOWCASE_CROP, true);

  lv_obj_t *img = lv_img_create(win);
  lv_img_set_src(img, &s_sprite.dsc);
  lv_img_set_antialias(img, false);
  lv_img_set_pivot(img, 0, 0);
  lv_img_set_zoom(img, 512);
  lv_obj_set_pos(img, -2 * ox, -2 * oy);  // relativo ao miolo da janela
}

// Dois tipos: um anel por cima da janela, com a borda inteira no tipo 2
// recortada na diagonal (inferior esquerdo → superior direito): só a metade de
// cima fica. (Borda parcial com raio no LVGL 8.3 não desenha a curva.)
lv_draw_mask_line_param_t s_ring_mask;  // precisa viver enquanto o anel desenha
int16_t s_ring_mask_id = LV_MASK_ID_INV;

void on_ring_draw(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_DRAW_MAIN_BEGIN) {
    lv_area_t a;
    lv_obj_get_coords(lv_event_get_target(e), &a);
    lv_draw_mask_line_points_init(&s_ring_mask, a.x1, a.y2, a.x2, a.y1,
                                  LV_DRAW_MASK_LINE_SIDE_TOP);
    s_ring_mask_id = lv_draw_mask_add(&s_ring_mask, nullptr);
  } else if (code == LV_EVENT_DRAW_MAIN_END && s_ring_mask_id != LV_MASK_ID_INV) {
    lv_draw_mask_free_param(&s_ring_mask);
    lv_draw_mask_remove_id(s_ring_mask_id);
    s_ring_mask_id = LV_MASK_ID_INV;
  }
}

void build_window_ring(const SpeciesRecord *rec) {
  if (!rec || rec->type2 == SpeciesType::None) return;
  lv_obj_t *ring = ui_box(s_card);
  lv_obj_set_size(ring, kWinW, kWinW);
  lv_obj_set_pos(ring, 0, kWinY);
  lv_obj_set_style_radius(ring, 8, 0);
  lv_obj_set_style_border_width(ring, kWinBorder, 0);
  lv_obj_set_style_border_color(
      ring, lv_color_mix(c(species_type_color(rec->type2)), c(theme::BG), LV_OPA_50), 0);
  lv_obj_add_event_cb(ring, on_ring_draw, LV_EVENT_DRAW_MAIN_BEGIN, nullptr);
  lv_obj_add_event_cb(ring, on_ring_draw, LV_EVENT_DRAW_MAIN_END, nullptr);
}

void build_stats(uint16_t n, const SpeciesRecord *rec) {
  lv_obj_t *col = ui_box(s_card);
  lv_obj_set_pos(col, kColX, kBodyY);
  lv_obj_set_size(col, kColW, kBodyH);
  lv_obj_set_style_pad_row(col, 6, 0);
  lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(col, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

  // Medidas e, embaixo, a geração (saiu do rodapé, que agora é do Claude).
  lv_obj_t *info = ui_box(col);
  lv_obj_set_size(info, kColW, LV_SIZE_CONTENT);
  lv_obj_set_style_pad_row(info, 2, 0);
  lv_obj_set_flex_flow(info, LV_FLEX_FLOW_COLUMN);
  char line[44];
  if (rec) {
    char h[12], w[12], hs[20], ws[20];
    format_tenths(rec->height_dm, tr(STR_DECIMAL_SEP), h, sizeof(h));
    format_tenths(rec->weight_hg, tr(STR_DECIMAL_SEP), w, sizeof(w));
    snprintf(hs, sizeof(hs), tr(STR_DETAIL_HEIGHT_FMT), h);
    snprintf(ws, sizeof(ws), tr(STR_DETAIL_WEIGHT_FMT), w);
    snprintf(line, sizeof(line), "%s · %s", hs, ws);
  } else {
    line[0] = '\0';
  }
  ui_label(info, &font_12_500, theme::MUTED, line);
  snprintf(line, sizeof(line), tr(STR_SC_GEN_FMT), dex_generation_roman(dex_generation_of(n)));
  ui_label(info, &font_12_500, theme::MUTED, line);

  // PS..Vel: rótulo, valor e barra na escala 150, >= 60 em accent.
  lv_obj_t *rows = ui_box(col);
  lv_obj_set_size(rows, kColW, SPECIES_STAT_COUNT * kStatH + (SPECIES_STAT_COUNT - 1) * kStatGap);
  for (int i = 0; i < SPECIES_STAT_COUNT; i++) {
    lv_coord_t y = i * (kStatH + kStatGap);
    uint8_t v = rec ? rec->stats[i] : 0;
    lv_obj_t *l = ui_label(rows, &font_12_600, theme::MUTED, tr(kStatLabels[i]));
    lv_obj_align(l, LV_ALIGN_TOP_LEFT, 0, y);
    lv_obj_t *val = ui_label(rows, &font_12_700, theme::TEXT, "");
    lv_label_set_text_fmt(val, "%u", v);
    lv_obj_set_width(val, kStatValueW);
    lv_obj_set_style_text_align(val, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_align(val, LV_ALIGN_TOP_LEFT, kStatLabelW + 4, y);
    lv_obj_t *bar = ui_progress(rows, kColW - kStatBarX, kStatBarH);
    lv_obj_align(bar, LV_ALIGN_TOP_LEFT, kStatBarX, y + (kStatH - kStatBarH) / 2);
    lv_obj_set_style_bg_color(bar, c(v >= 60 ? theme::ACCENT : theme::TEXT), LV_PART_INDICATOR);
    ui_progress_set(bar, v > 150 ? 150 : v, 150);
  }
}

// Conteúdo do rodapé (o contêiner s_foot sobrevive; só os filhos são refeitos).
void fill_footer() {
  lv_obj_clean(s_foot);
  ClaudeState st = claude_state();
  const BuddySnapshot &snap = claude_snapshot();

  uint32_t dot_color = theme::FAINT, text_color = theme::MUTED;
  bool filled = false;
  char label[32];
  switch (st) {
    case ClaudeState::Disconnected:
      snprintf(label, sizeof(label), "%s", tr(STR_CL_DISCONNECTED));
      break;
    case ClaudeState::Idle:
      dot_color = theme::MUTED;
      snprintf(label, sizeof(label), "%s", tr(STR_CL_IDLE));
      break;
    case ClaudeState::Running:
      dot_color = theme::RUNNING;
      text_color = theme::TEXT;
      filled = true;
      snprintf(label, sizeof(label), tr(STR_CL_RUNNING_FMT), (unsigned)snap.running);
      break;
    case ClaudeState::Waiting:
      dot_color = theme::WAITING;
      filled = true;
      // Sem prompt (o Claude Code manda só o waiting), a L02 não abre: avisa
      // que a decisão é no computador.
      if (claude_prompt().open()) {
        text_color = theme::TEXT;
        snprintf(label, sizeof(label), "%s", tr(STR_CL_WAITING));
      } else {
        text_color = theme::WAITING;
        snprintf(label, sizeof(label), "%s", tr(STR_CL_APPROVE_ON_PC));
      }
      break;
  }

  lv_obj_t *dot = ui_box(s_foot);
  lv_obj_set_size(dot, 8, 8);
  lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
  if (filled) {
    lv_obj_set_style_bg_color(dot, c(dot_color), 0);
    lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
  } else {
    lv_obj_set_style_border_width(dot, 2, 0);
    lv_obj_set_style_border_color(dot, c(dot_color), 0);
  }
  lv_obj_t *l = ui_label(s_foot, &font_12_700, text_color, label);
  lv_obj_set_flex_grow(l, 1);
  lv_label_set_long_mode(l, LV_LABEL_LONG_DOT);

  if (st == ClaudeState::Disconnected) return;  // sem link: só o estado
  const TokenLedger &ledger = claude_ledger();
  char lvl[16], today[16];
  snprintf(lvl, sizeof(lvl), tr(STR_CL_LEVEL_FMT), (unsigned)ledger.level());
  format_tokens(snap.tokens_today, tr(STR_DECIMAL_SEP), today, sizeof(today));
  ui_label(s_foot, &font_12_700, theme::MUTED, lvl);
  lv_obj_t *bar = ui_progress(s_foot, kLevelBarW, 4);
  ui_progress_set(bar, ledger.progress_permille(), 1000);
  ui_label(s_foot, &font_12_700, theme::TEXT, today);
  ui_label(s_foot, &font_12_500, theme::MUTED, tr(STR_CL_TODAY));
}

void build_footer() {
  s_foot = ui_box(s_card);
  lv_obj_set_size(s_foot, kInnerW, kFootH);
  lv_obj_align(s_foot, LV_ALIGN_BOTTOM_LEFT, 0, 0);
  lv_obj_set_style_border_side(s_foot, LV_BORDER_SIDE_TOP, 0);
  lv_obj_set_style_border_width(s_foot, 1, 0);
  lv_obj_set_style_border_color(s_foot, c(theme::LINE), 0);
  lv_obj_set_style_pad_top(s_foot, 4, 0);
  lv_obj_set_style_pad_column(s_foot, 6, 0);
  lv_obj_set_flex_flow(s_foot, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(s_foot, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  fill_footer();
}

void show(uint16_t n) {
  uint32_t t0 = lv_tick_get();
  // O cartão sai antes do sprite ser relido: o lv_img aponta pro buffer fixo.
  lv_obj_clean(s_card);
  s_foot = nullptr;

  SpeciesRecord rec;
  bool have = species_repo_get(n, &rec);
  int8_t dx, dy;
  showcase_shift(s_shift_k++, &dx, &dy);
  lv_obj_set_pos(s_card, kCardX + dx, kCardY + dy);

  const SpeciesRecord *r = have ? &rec : nullptr;
  build_header(n, r);
  build_window(n, r);
  build_window_ring(r);  // depois da janela: desenha por cima do sprite
  build_stats(n, r);
  build_footer();

  LOG_D("showcase", "#%u em %ums, maior_bloco=%u", n, lv_tick_elaps(t0),
        heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
}

void next() {
  if (s_count == 0) return;
  if (settings().showcase_random) {
    s_pos = showcase_pick(s_list, s_count, s_shown, s_last_n, esp_random());
  } else {
    s_pos = showcase_next(s_pos < s_count ? s_pos : -1, s_count, false, 0);
  }
  s_last_n = s_list[s_pos];
  show(s_last_n);
}

void on_tick(lv_timer_t *) { next(); }

void on_tap(lv_event_t *) {
  next();
  if (s_timer) lv_timer_reset(s_timer);
}

void on_delete(lv_event_t *) {
  if (s_timer) lv_timer_del(s_timer);
  s_timer = nullptr;
  s_card = s_foot = nullptr;
  s_scr = nullptr;
}

}  // namespace

lv_obj_t *showcase_screen_create() {
  lv_obj_t *scr = ui_screen();
  s_scr = scr;
  lv_obj_add_event_cb(scr, on_delete, LV_EVENT_DELETE, nullptr);
  ui_on_click(scr, on_tap);

  s_count = species_repo_count();
  for (uint16_t i = 0; i < s_count; i++) s_list[i] = i + 1;

  s_card = ui_box(scr);
  lv_obj_set_size(s_card, kCardW, kCardH);
  lv_obj_set_style_pad_ver(s_card, kPadV, 0);
  lv_obj_set_style_pad_hor(s_card, kPadH, 0);

  next();
  s_timer = lv_timer_create(on_tick, (uint32_t)settings().showcase_interval_s * 1000u, nullptr);
  return scr;
}

void showcase_update_footer() {
  if (s_scr && s_foot) fill_footer();
}
