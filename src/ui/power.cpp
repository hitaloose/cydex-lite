#include "power.h"

#include <Arduino.h>
#include <lvgl.h>

#include "app/claude_link.h"
#include "data/Settings.h"
#include "hal/backlight.h"
#include "hal/speaker.h"
#include "hal/touch.h"
#include "logic/Format.h"
#include "util/log.h"

static bool s_night = false;
static uint32_t s_checked_at = 0;

static void apply() {
  const AppSettings &st = settings();
  backlight_set(s_night ? st.brightness_night : st.brightness);
}

static void update(bool force) {
  s_checked_at = millis();
  int64_t epoch = 0;
  int32_t tz = 0;
  const AppSettings &st = settings();
  bool night = claude_clock(&epoch, &tz) &&
               local_is_night(epoch, tz, st.night_from_h, st.night_to_h);
  if (night == s_night && !force) return;
  if (night != s_night) LOG_I("power", "%s", night ? "anoiteceu" : "amanheceu");
  s_night = night;
  apply();
}

static void on_press() {
  if (settings().sound) speaker_click();
}

void power_init() {
  speaker_init();
  touch_set_press_hook(on_press);
  update(true);
}

void power_tick() {
  speaker_update();
  if (millis() - s_checked_at >= 60000u) update(false);
}

void power_clock_changed() { update(false); }

void power_wake() {
  apply();
  lv_disp_trig_activity(nullptr);
}
