#include "led.h"

#include <Arduino.h>

#include "board_pins.h"

static bool s_on = false;
static bool s_lit = false;
static uint32_t s_at = 0;

static void red(bool lit) {
  s_lit = lit;
  digitalWrite(PIN_LED_RED, lit ? LOW : HIGH);
}

void led_init() {
  pinMode(PIN_LED_RED, OUTPUT);
  pinMode(PIN_LED_GREEN, OUTPUT);
  pinMode(PIN_LED_BLUE, OUTPUT);
  digitalWrite(PIN_LED_RED, HIGH);
  digitalWrite(PIN_LED_GREEN, HIGH);
  digitalWrite(PIN_LED_BLUE, HIGH);
}

void led_attention(bool on) {
  s_on = on;
  s_at = millis();
  red(on);
}

void led_update() {
  if (!s_on || millis() - s_at < 500) return;
  s_at = millis();
  red(!s_lit);
}
