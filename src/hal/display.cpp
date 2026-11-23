#include "display.h"

#include "board_pins.h"
#include "util/early_alloc.h"
#include "util/log.h"

static const char *TAG = "display";

TFT_eSPI tft;

// Sem PSRAM: buffer parcial (320 x 24 linhas = 15 KB) em DRAM interna.
// Nunca um frame buffer inteiro (150 KB).
#define DISP_BUF_LINES 24

static lv_disp_draw_buf_t draw_buf;
// No heap (util/early_alloc), pedido antes do NimBLE e do FATFS.
static constexpr size_t kDispBufPx = BOARD_DISPLAY_WIDTH * DISP_BUF_LINES;
static lv_color_t *disp_buf = nullptr;
static lv_disp_drv_t disp_drv;

// Print pela serial (display_capture_to_serial): enquanto ligado, cada faixa
// que o flush_cb manda pro painel também sai na Serial.
static bool s_capture = false;

static void write_u16(uint16_t v) {
  uint8_t b[2] = {(uint8_t)(v & 0xFF), (uint8_t)(v >> 8)};
  Serial.write(b, 2);
}

void display_init() {
  tft.init();
  tft.setRotation(1);  // paisagem
  tft.fillScreen(TFT_BLACK);
  LOG_I(TAG, "TFT ok — %dx%d, rotation=1", tft.width(), tft.height());
}

void display_self_test() {
  const uint16_t colors[] = {TFT_RED, TFT_GREEN, TFT_BLUE, TFT_YELLOW, TFT_WHITE};
  const int n = sizeof(colors) / sizeof(colors[0]);
  const int w = tft.width() / n;
  for (int i = 0; i < n; i++) {
    tft.fillRect(i * w, 0, w, tft.height(), colors[i]);
  }
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  tft.setTextSize(2);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("CYDex", tft.width() / 2, tft.height() / 2);
}

static void flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p) {
  uint32_t w = area->x2 - area->x1 + 1;
  uint32_t h = area->y2 - area->y1 + 1;
  tft.startWrite();
  tft.setAddrWindow(area->x1, area->y1, w, h);
  tft.pushColors((uint16_t *)&color_p->full, w * h, true);
  tft.endWrite();
  if (s_capture) {
    // "CYSA" + x, y, w, h (u16 LE) + RGB565 nativo (LV_COLOR_16_SWAP 0 = LE)
    // + soma dos pixels (u16): log de outra task no meio da faixa é detectado.
    Serial.write((const uint8_t *)"CYSA", 4);
    write_u16(area->x1);
    write_u16(area->y1);
    write_u16(w);
    write_u16(h);
    Serial.write((const uint8_t *)color_p, w * h * sizeof(lv_color_t));
    uint16_t sum = 0;
    for (uint32_t i = 0; i < w * h; i++) sum += color_p[i].full;
    write_u16(sum);
  }
  lv_disp_flush_ready(drv);
}

void display_lvgl_init() {
  disp_buf = (lv_color_t *)early_alloc(kDispBufPx * sizeof(lv_color_t));
  lv_disp_draw_buf_init(&draw_buf, disp_buf, nullptr, kDispBufPx);
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = BOARD_DISPLAY_WIDTH;
  disp_drv.ver_res = BOARD_DISPLAY_HEIGHT;
  disp_drv.flush_cb = flush_cb;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);
  LOG_I(TAG, "LVGL: buffer parcial de %d linhas (%u bytes)", DISP_BUF_LINES,
        (unsigned)(kDispBufPx * sizeof(lv_color_t)));
}

void display_capture_to_serial(uint16_t y0, uint16_t y1) {
  // Invalidar a área do display redesenha todas as camadas nela (inclusive a
  // faixa de Aviso no lv_layer_top). Tela toda: ~150 KB a 115200 bps, uns 13 s parado.
  if (y1 >= BOARD_DISPLAY_HEIGHT) y1 = BOARD_DISPLAY_HEIGHT - 1;
  if (y0 > y1) return;
  Serial.flush();
  s_capture = true;
  lv_area_t a = {0, (lv_coord_t)y0, BOARD_DISPLAY_WIDTH - 1, (lv_coord_t)y1};
  _lv_inv_area(lv_disp_get_default(), &a);
  lv_refr_now(nullptr);
  s_capture = false;
  Serial.write((const uint8_t *)"CYSE", 4);
  Serial.flush();
}
