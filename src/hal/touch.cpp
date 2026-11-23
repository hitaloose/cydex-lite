#include "touch.h"

#include <Arduino.h>
#include <lvgl.h>

#include "board_pins.h"
#include "util/log.h"

static const char *TAG = "touch";

// Protocolo do XPT2046 portado do firmware Bruce (CYD28_TouchscreenR.h)
// via PokEsp. O touch fica num barramento próprio, sem SPI de hardware,
// por isso o bit-bang.
//
// As faixas brutas são propriedade física do filme resistivo e não mudam
// com a rotação. Em paisagem (setRotation(1)) o PokEsp usava o eixo bruto
// direto, sem swap nem inversão. CONFIRMAR NA PLACA no M0.
static const int32_t CAL_X_MIN = 185;
static const int32_t CAL_X_MAX = 3700;
static const int32_t CAL_Y_MIN = 280;
static const int32_t CAL_Y_MAX = 3850;
static const int32_t Z_THRESHOLD = 300;

static const bool TOUCH_SWAP_XY = false;
static const bool TOUCH_INVERT_X = false;
static const bool TOUCH_INVERT_Y = false;

static lv_indev_drv_t indev_drv;

static uint8_t spi_xfer(uint8_t out) {
  uint8_t in = 0;
  for (int8_t bit = 7; bit >= 0; bit--) {
    digitalWrite(PIN_TOUCH_MOSI, ((out >> bit) & 1) ? HIGH : LOW);
    digitalWrite(PIN_TOUCH_CLK, HIGH);
    in = (uint8_t)((in << 1) | digitalRead(PIN_TOUCH_MISO));
    digitalWrite(PIN_TOUCH_CLK, LOW);
  }
  return in;
}

// Comando seguinte emendado com o resultado da conversão anterior (pipeline
// do XPT2046 em bursts de 16 clocks).
static uint16_t spi_xfer16(uint16_t v) {
  uint8_t hi = spi_xfer((uint8_t)(v >> 8));
  uint8_t lo = spi_xfer((uint8_t)(v & 0xFF));
  return (uint16_t)((hi << 8) | lo);
}

static int16_t best_two_avg(int16_t a, int16_t b, int16_t c) {
  int16_t ab = (int16_t)abs(a - b), ac = (int16_t)abs(a - c), bc = (int16_t)abs(c - b);
  if (ab <= ac && ab <= bc) return (int16_t)((a + b) >> 1);
  if (ac <= ab && ac <= bc) return (int16_t)((a + c) >> 1);
  return (int16_t)((b + c) >> 1);
}

static bool read_raw(int16_t *outX, int16_t *outY) {
  digitalWrite(PIN_TOUCH_CS, LOW);

  spi_xfer(0xB1);
  int32_t z = (int32_t)(spi_xfer16(0xC1) >> 3) + 4095;
  z -= (spi_xfer16(0x91) >> 3);

  int16_t data[6] = {0, 0, 0, 0, 0, 0};
  bool valid = z >= Z_THRESHOLD;
  if (valid) {
    spi_xfer16(0x91);  // 1a leitura é sempre ruidosa
    data[0] = (int16_t)(spi_xfer16(0xD1) >> 3);
    data[1] = (int16_t)(spi_xfer16(0x91) >> 3);
    data[2] = (int16_t)(spi_xfer16(0xD1) >> 3);
    data[3] = (int16_t)(spi_xfer16(0x91) >> 3);
  }
  data[4] = (int16_t)(spi_xfer16(0xD0) >> 3);
  data[5] = (int16_t)(spi_xfer16(0) >> 3);

  digitalWrite(PIN_TOUCH_CS, HIGH);

  if (!valid) return false;
  *outX = best_two_avg(data[0], data[2], data[4]);
  *outY = best_two_avg(data[1], data[3], data[5]);
  return true;
}

static int32_t clamp(int32_t v, int32_t lo, int32_t hi) { return v < lo ? lo : (v > hi ? hi : v); }

bool touch_read(int16_t *x, int16_t *y) {
  int16_t rawX, rawY;
  if (!read_raw(&rawX, &rawY)) return false;

  int32_t rx = TOUCH_SWAP_XY ? rawY : rawX;
  int32_t ry = TOUCH_SWAP_XY ? rawX : rawY;
  int32_t xmin = TOUCH_SWAP_XY ? CAL_Y_MIN : CAL_X_MIN;
  int32_t xmax = TOUCH_SWAP_XY ? CAL_Y_MAX : CAL_X_MAX;
  int32_t ymin = TOUCH_SWAP_XY ? CAL_X_MIN : CAL_Y_MIN;
  int32_t ymax = TOUCH_SWAP_XY ? CAL_X_MAX : CAL_Y_MAX;

  int32_t xx = (rx - xmin) * BOARD_DISPLAY_WIDTH / (xmax - xmin);
  int32_t yy = (ry - ymin) * BOARD_DISPLAY_HEIGHT / (ymax - ymin);
  if (TOUCH_INVERT_X) xx = (BOARD_DISPLAY_WIDTH - 1) - xx;
  if (TOUCH_INVERT_Y) yy = (BOARD_DISPLAY_HEIGHT - 1) - yy;

  *x = (int16_t)clamp(xx, 0, BOARD_DISPLAY_WIDTH - 1);
  *y = (int16_t)clamp(yy, 0, BOARD_DISPLAY_HEIGHT - 1);
  return true;
}

void touch_init() {
  pinMode(PIN_TOUCH_MOSI, OUTPUT);
  pinMode(PIN_TOUCH_MISO, INPUT);
  pinMode(PIN_TOUCH_CLK, OUTPUT);
  pinMode(PIN_TOUCH_CS, OUTPUT);
  digitalWrite(PIN_TOUCH_CLK, LOW);
  digitalWrite(PIN_TOUCH_CS, HIGH);
  LOG_I(TAG, "bit-bang CS=%d CLK=%d MOSI=%d MISO=%d", PIN_TOUCH_CS, PIN_TOUCH_CLK,
        PIN_TOUCH_MOSI, PIN_TOUCH_MISO);
}

static TouchFilter s_filter = nullptr;
static TouchPressHook s_press_hook = nullptr;
static bool s_was_pressed = false;

void touch_set_filter(TouchFilter f) { s_filter = f; }
void touch_set_press_hook(TouchPressHook h) { s_press_hook = h; }

static void read_cb(lv_indev_drv_t *drv, lv_indev_data_t *data) {
  (void)drv;
  int16_t x, y;
  bool pressed = touch_read(&x, &y);
  if (s_filter && !s_filter(pressed)) pressed = false;
  if (pressed && !s_was_pressed && s_press_hook) s_press_hook();
  s_was_pressed = pressed;
  if (pressed) {
    data->state = LV_INDEV_STATE_PRESSED;
    data->point.x = x;
    data->point.y = y;
  } else {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}

void touch_lvgl_init() {
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = read_cb;
  lv_indev_drv_register(&indev_drv);
}
