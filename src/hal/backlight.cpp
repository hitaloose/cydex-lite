#include "backlight.h"

#include <Arduino.h>

#include "board_pins.h"

static const int kChannel = 0;
static const int kFreqHz = 5000;
static const int kResolutionBits = 8;
static uint8_t s_percent = 100;

void backlight_init() {
  ledcSetup(kChannel, kFreqHz, kResolutionBits);
  ledcAttachPin(PIN_TFT_BL, kChannel);
  backlight_set(100);
}

void backlight_set(uint8_t percent) {
  if (percent > 100) percent = 100;
  s_percent = percent;
  ledcWrite(kChannel, (uint32_t)percent * 255 / 100);
}

uint8_t backlight_get() { return s_percent; }
