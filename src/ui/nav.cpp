#include "nav.h"

#include <lvgl.h>

#include "ui/screens/screens.h"

static Screen s_current = Screen::Boot;

static lv_obj_t *build(Screen s) {
  switch (s) {
    case Screen::Boot: return boot_screen_create();
    case Screen::SdError: return sd_error_screen_create();
    case Screen::Showcase: return showcase_screen_create();
  }
  return boot_screen_create();
}

void nav_go(Screen s) {
  s_current = s;
  // Tela vazia no meio: a anterior sai (LV_EVENT_DELETE solta sprite e
  // timers) antes de a próxima ser montada. Tudo síncrono, não pisca.
  lv_obj_t *blank = lv_obj_create(nullptr);
  lv_scr_load_anim(blank, LV_SCR_LOAD_ANIM_NONE, 0, 0, true);
  lv_scr_load_anim(build(s), LV_SCR_LOAD_ANIM_NONE, 0, 0, true);
}

Screen nav_current() { return s_current; }
