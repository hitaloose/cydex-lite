#pragma once

// Pinout do CYD usado pelo CYDex: ESP32-2432S028 (SEM o "R"), controlador
// ST7789, sem PSRAM. Tudo confirmado fisicamente no PokEsp (mesma placa).
// Único arquivo que conhece números de pino.
//
// Três barramentos independentes, sem SPI compartilhado:
//   display  -> HSPI (12/13/14/15)
//   touch    -> bit-bang próprio (25/32/39/33), não está em SPI de hardware
//   microSD  -> VSPI default (18/19/23/5)

// ---- Display ST7789 (HSPI) ----
#define PIN_TFT_MISO 12
#define PIN_TFT_MOSI 13
#define PIN_TFT_SCLK 14
#define PIN_TFT_CS   15
#define PIN_TFT_DC   2
#define PIN_TFT_RST  -1  // ligado ao EN
#define PIN_TFT_BL   21  // backlight, PWM via ledc

// ---- Touch XPT2046 (bit-bang) ----
#define PIN_TOUCH_MOSI 32
#define PIN_TOUCH_MISO 39
#define PIN_TOUCH_CLK  25
#define PIN_TOUCH_CS   33
#define PIN_TOUCH_IRQ  36

// ---- microSD (VSPI) ----
#define PIN_SD_CS   5
#define PIN_SD_MOSI 23
#define PIN_SD_MISO 19
#define PIN_SD_SCK  18

// ---- Onboard ----
#define PIN_LED_RED   4   // RGB comum-anodo: HIGH apaga
#define PIN_LED_GREEN 16
#define PIN_LED_BLUE  17
#define PIN_LDR       34

// Conector de alto-falante (P4 "SPEAK"), via amplificador onboard.
#define PIN_SPEAKER 26

#define BOARD_HAS_PSRAM 0

// Paisagem (tft.setRotation(1)).
#define BOARD_DISPLAY_WIDTH  320
#define BOARD_DISPLAY_HEIGHT 240
