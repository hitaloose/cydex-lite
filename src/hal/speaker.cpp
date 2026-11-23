#include "speaker.h"

#include <Arduino.h>

#include "board_pins.h"

// Canal 2 = timer 1 do ledc. O backlight está no canal 0 (timer 0); canais
// 0 e 1 dividem o timer, e mudar a frequência ali mexeria no PWM da tela.
static const int kChannel = 2;
static const uint32_t kToneHz = 2400;
static const uint32_t kClickMs = 12;
static uint32_t s_until = 0;
static bool s_on = false;

// Bipe do aviso: tom, pausa, tom mais agudo. {Hz (0 = silêncio), ms}.
struct Step {
  uint16_t hz, ms;
};
static const Step kChime[] = {{1800, 70}, {0, 50}, {2400, 90}};
static const uint8_t kChimeSteps = sizeof(kChime) / sizeof(kChime[0]);
static uint8_t s_step = kChimeSteps;  // = parado

static void play(uint16_t hz, uint16_t ms) {
  if (hz) {
    ledcWriteTone(kChannel, hz);
    ledcWrite(kChannel, 48);
  } else {
    ledcWrite(kChannel, 0);
  }
  s_until = millis() + ms;
  s_on = true;
}

void speaker_init() {
  ledcSetup(kChannel, kToneHz, 8);
  ledcAttachPin(PIN_SPEAKER, kChannel);
  ledcWrite(kChannel, 0);
}

void speaker_click() {
  if (s_step < kChimeSteps) return;  // não corta o bipe do aviso
  ledcWriteTone(kChannel, kToneHz);
  ledcWrite(kChannel, 32);  // duty baixo: clique discreto
  s_until = millis() + kClickMs;
  s_on = true;
}

void speaker_chime() {
  s_step = 0;
  play(kChime[0].hz, kChime[0].ms);
}

void speaker_update() {
  if (!s_on || (int32_t)(millis() - s_until) < 0) return;
  if (s_step + 1 < kChimeSteps) {
    s_step++;
    play(kChime[s_step].hz, kChime[s_step].ms);
    return;
  }
  s_step = kChimeSteps;
  ledcWrite(kChannel, 0);
  s_on = false;
}
