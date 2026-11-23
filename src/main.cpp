#include <Arduino.h>
#include <lvgl.h>

#include "app/claude_link.h"
#include "board_pins.h"
#include "data/Settings.h"
#include "data/SpriteLoader.h"
#include "hal/backlight.h"
#include "hal/display.h"
#include "hal/led.h"
#include "hal/sd.h"
#include "hal/touch.h"
#include "ui/claude_ui.h"
#include "ui/nav.h"
#include "ui/power.h"
#include "util/early_alloc.h"
#include "util/log.h"

static const char *TAG = "main";

// Sem parser de saves (o que pedia 32 KB no CYDex): a cadeia mais funda é o
// LVGL desenhando a vitrine. 12 KB dá folga sobre os 8 KB padrão; medir com
// LOG_LEVEL=2 (pilha_livre_min) antes de mexer.
size_t getArduinoLoopTaskStackSize(void) { return 12288; }

static void log_mem(const char *what) {
  LOG_I(TAG, "%s: heap=%u maior_bloco=%u", what, ESP.getFreeHeap(),
        heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
}

// Comandos de linha pela serial. "shot" = print da tela (tools/cydex_screenshot.py).
static void serial_cmd_tick() {
  static char line[24];
  static size_t len = 0;
  while (Serial.available() > 0) {
    char ch = (char)Serial.read();
    if (ch == '\r') continue;
    if (ch != '\n') {
      if (len < sizeof(line) - 1) line[len++] = ch;
      continue;
    }
    line[len] = '\0';
    len = 0;
    unsigned y0 = 0, y1 = 0;
    if (strcmp(line, "shot") == 0) display_capture_to_serial(0, BOARD_DISPLAY_HEIGHT - 1);
    else if (sscanf(line, "shot %u %u", &y0, &y1) == 2) display_capture_to_serial(y0, y1);
  }
}

void setup() {
  Serial.begin(115200);
  delay(100);
  led_init();
  LOG_I(TAG, "CYDex Lite — chip=%s %dMHz", ESP.getChipModel(), ESP.getCpuFreqMHz());
  log_mem("boot");

  // Ordem (o heap do ESP32 é picado em regiões; o maior bloco no boot ~47 KB):
  // buffer do display -> LVGL -> sprite 18 KB -> SD (FATFS ~17 KB contíguos)
  // -> NimBLE (~19 KB no init). Mudou algo grande? Conferir o maior_bloco.
  early_alloc_init();
  display_init();
  backlight_init();
  lv_init();
  display_lvgl_init();
  touch_init();
  touch_lvgl_init();
  settings_init();
  sprite_reserve();
  log_mem("reservas");
  sd_init();  // o Boot encontra montado; sem cartão, ele tenta de novo
  log_mem("sd");
  claude_init();  // NimBLE + Nordic UART
  log_mem("ble");
  power_init();
  claude_ui_init();
  nav_go(Screen::Boot);
}

void loop() {
  lv_timer_handler();
#if LOG_LEVEL >= 2
  static uint32_t s_mem_at = 0;
  if (millis() - s_mem_at > 10000) {
    s_mem_at = millis();
    lv_mem_monitor_t m;
    lv_mem_monitor(&m);
    LOG_D("mem", "pilha_livre_min=%u lv_max_usado=%u lv_frag=%u%% heap=%u maior_bloco=%u min_heap=%u",
          (unsigned)uxTaskGetStackHighWaterMark(nullptr), (unsigned)m.max_used, m.frag_pct,
          ESP.getFreeHeap(), heap_caps_get_largest_free_block(MALLOC_CAP_8BIT),
          ESP.getMinFreeHeap());
  }
#endif
  claude_tick();
  power_tick();
  led_update();
  serial_cmd_tick();
  delay(5);
}
