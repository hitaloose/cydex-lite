#include "widgets.h"

#include "ui/icons/icons.h"
#include "ui/theme.h"

lv_obj_t *ui_screen() {
  lv_obj_t *s = lv_obj_create(nullptr);
  lv_obj_remove_style_all(s);
  lv_obj_set_size(s, LV_HOR_RES, LV_VER_RES);
  lv_obj_set_style_bg_color(s, c(theme::BG), 0);
  lv_obj_set_style_bg_opa(s, LV_OPA_COVER, 0);
  lv_obj_set_style_text_color(s, c(theme::TEXT), 0);
  lv_obj_set_style_text_font(s, &font_14_500, 0);
  lv_obj_clear_flag(s, LV_OBJ_FLAG_SCROLLABLE);
  return s;
}

lv_obj_t *ui_box(lv_obj_t *parent) {
  lv_obj_t *o = lv_obj_create(parent);
  lv_obj_remove_style_all(o);
  lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
  return o;
}

lv_obj_t *ui_label(lv_obj_t *parent, const lv_font_t *font, uint32_t color, const char *text) {
  lv_obj_t *l = lv_label_create(parent);
  lv_obj_set_style_text_font(l, font, 0);
  lv_obj_set_style_text_color(l, c(color), 0);
  lv_label_set_text(l, text);
  return l;
}

lv_obj_t *ui_icon(lv_obj_t *parent, const lv_img_dsc_t *icon, uint32_t color) {
  lv_obj_t *img = lv_img_create(parent);
  lv_img_set_src(img, icon);
  lv_obj_set_style_img_recolor(img, c(color), 0);
  lv_obj_set_style_img_recolor_opa(img, LV_OPA_COVER, 0);
  lv_obj_clear_flag(img, LV_OBJ_FLAG_CLICKABLE);
  return img;
}

lv_obj_t *ui_logo(lv_obj_t *parent, lv_coord_t size) {
  // SVG do canvas, viewBox 28: barras em y=3/11/19, altura 6, larguras 24/18/11.
  lv_obj_t *box = ui_box(parent);
  lv_obj_set_size(box, size, size);
  struct Bar {
    int y, w;
    uint32_t color;
  };
  static const Bar bars[] = {{3, 24, theme::ACCENT}, {11, 18, theme::TEXT}, {19, 11, theme::MUTED}};
  for (const Bar &b : bars) {
    lv_obj_t *r = ui_box(box);
    lv_obj_set_pos(r, 2 * size / 28, b.y * size / 28);
    lv_obj_set_size(r, b.w * size / 28, (6 * size + 14) / 28);
    lv_obj_set_style_radius(r, (2 * size + 14) / 28, 0);
    lv_obj_set_style_bg_color(r, c(b.color), 0);
    lv_obj_set_style_bg_opa(r, LV_OPA_COVER, 0);
  }
  return box;
}

lv_obj_t *ui_progress(lv_obj_t *parent, lv_coord_t w, lv_coord_t h) {
  lv_obj_t *bar = lv_bar_create(parent);
  lv_obj_remove_style_all(bar);
  lv_obj_set_size(bar, w, h);
  lv_obj_set_style_radius(bar, h / 2, 0);
  lv_obj_set_style_bg_color(bar, c(theme::SURFACE_2), 0);
  lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(bar, h / 2, LV_PART_INDICATOR);
  lv_obj_set_style_bg_color(bar, c(theme::ACCENT), LV_PART_INDICATOR);
  lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, LV_PART_INDICATOR);
  lv_obj_set_style_anim_time(bar, 0, 0);
  lv_bar_set_range(bar, 0, 1000);
  lv_bar_set_value(bar, 0, LV_ANIM_OFF);
  return bar;
}

void ui_progress_set(lv_obj_t *bar, int32_t value, int32_t max) {
  int32_t v = max > 0 ? value * 1000 / max : 0;
  lv_bar_set_value(bar, v, LV_ANIM_OFF);
}

void ui_on_click(lv_obj_t *obj, lv_event_cb_t cb, void *user) {
  lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
  lv_obj_add_event_cb(obj, cb, LV_EVENT_CLICKED, user);
}

lv_obj_t *ui_chip(lv_obj_t *parent, const char *text, uint32_t bg, uint32_t fg, lv_coord_t pad_hor) {
  lv_obj_t *chip = ui_box(parent);
  lv_obj_set_size(chip, LV_SIZE_CONTENT, 20);
  lv_obj_set_style_radius(chip, 10, 0);
  lv_obj_set_style_bg_color(chip, c(bg), 0);
  lv_obj_set_style_bg_opa(chip, LV_OPA_COVER, 0);
  lv_obj_set_style_pad_hor(chip, pad_hor, 0);
  lv_obj_t *l = ui_label(chip, &font_12_700, fg, text);
  lv_obj_align(l, LV_ALIGN_LEFT_MID, 0, 0);
  return chip;
}

void ui_label_clamp(lv_obj_t *label, lv_coord_t w, uint8_t max_lines) {
  const lv_font_t *font = lv_obj_get_style_text_font(label, LV_PART_MAIN);
  lv_coord_t line_space = lv_obj_get_style_text_line_space(label, LV_PART_MAIN);
  lv_point_t size;
  lv_txt_get_size(&size, lv_label_get_text(label), font, 0, line_space, w, LV_TEXT_FLAG_NONE);
  lv_coord_t line_h = lv_font_get_line_height(font);
  lv_coord_t max_h = (lv_coord_t)(line_h * max_lines + line_space * (max_lines - 1));
  lv_obj_set_width(label, w);
  lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);
  lv_obj_set_height(label, size.y < max_h ? size.y : max_h);
}

lv_obj_t *ui_button(lv_obj_t *parent, const lv_img_dsc_t *icon, const char *text, uint32_t bg,
                    uint32_t fg) {
  lv_obj_t *btn = ui_box(parent);
  lv_obj_set_size(btn, LV_SIZE_CONTENT, 44);
  lv_obj_set_style_pad_hor(btn, 24, 0);
  lv_obj_set_style_radius(btn, 8, 0);
  lv_obj_set_style_bg_color(btn, c(bg), 0);
  lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
  lv_obj_set_flex_flow(btn, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(btn, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_column(btn, 6, 0);
  if (icon) ui_icon(btn, icon, fg);
  ui_label(btn, &font_14_700, fg, text);
  return btn;
}
