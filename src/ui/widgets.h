#pragma once

#include <lvgl.h>

// Peças pequenas reaproveitadas pelas telas. Tudo sem animação e sem scroll.

// Tela vazia com o fundo do tema.
lv_obj_t *ui_screen();
// Container "cru": sem estilo, sem scroll, sem clique.
lv_obj_t *ui_box(lv_obj_t *parent);
lv_obj_t *ui_label(lv_obj_t *parent, const lv_font_t *font, uint32_t color, const char *text);
// Ícone A8 (src/ui/icons) pintado com `color`.
lv_obj_t *ui_icon(lv_obj_t *parent, const lv_img_dsc_t *icon, uint32_t color);
// Logo de 3 barras (accent / text / muted), `size` px de lado.
lv_obj_t *ui_logo(lv_obj_t *parent, lv_coord_t size);
// Barra de progresso fina (trilho surface-2, preenchimento accent).
lv_obj_t *ui_progress(lv_obj_t *parent, lv_coord_t w, lv_coord_t h);
void ui_progress_set(lv_obj_t *bar, int32_t value, int32_t max);
// Label de largura `w` que quebra linha e para em `max_lines` com "…".
void ui_label_clamp(lv_obj_t *label, lv_coord_t w, uint8_t max_lines);
// Torna `obj` clicável, chamando `cb` no clique (com `user` em event user_data).
void ui_on_click(lv_obj_t *obj, lv_event_cb_t cb, void *user = nullptr);
// Chip arredondado de 20 px (tipos da Vitrine, "+n" do pedido).
lv_obj_t *ui_chip(lv_obj_t *parent, const char *text, uint32_t bg, uint32_t fg,
                  lv_coord_t pad_hor = 8);
// Botão de 44 px com ícone + texto (Tentar de novo, Negar, Aprovar).
lv_obj_t *ui_button(lv_obj_t *parent, const lv_img_dsc_t *icon, const char *text, uint32_t bg,
                    uint32_t fg);
