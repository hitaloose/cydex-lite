#pragma once

// User_Setup do TFT_eSPI, aplicado via `-include` no platformio.ini.
// Valores herdados do PokEsp, validados na placa física.

#define USER_SETUP_LOADED 1

#include "board_pins.h"

// Display na HSPI; o SD fica com a VSPI default.
#define USE_HSPI_PORT

// O controlador real é ST7789 (a serigrafia sugere ILI9341, mas a init do
// ILI9341 deu cores em negativo). Com ST7789 puro os canais saem R<->B
// trocados e invertidos: BGR corrige a troca e INVERSION_OFF anula o INVON
// incondicional da sequência de init.
#define ST7789_DRIVER 1
#define TFT_WIDTH  240
#define TFT_HEIGHT 320
#define TFT_RGB_ORDER TFT_BGR
#define TFT_INVERSION_OFF

#define TFT_MISO PIN_TFT_MISO
#define TFT_MOSI PIN_TFT_MOSI
#define TFT_SCLK PIN_TFT_SCLK
#define TFT_CS   PIN_TFT_CS
#define TFT_DC   PIN_TFT_DC
#define TFT_RST  PIN_TFT_RST
// TFT_BL não é definido: o backlight é controlado por PWM em hal/backlight.

// O touch embutido do TFT_eSPI não é usado (XPT2046 em barramento próprio),
// então o aviso de TOUCH_CS ausente é esperado.
#define DISABLE_ALL_LIBRARY_WARNINGS

// Só a fonte GLCD pro self-test; a UI usa fontes do LVGL.
#define LOAD_GLCD 1

#define SPI_FREQUENCY      55000000
#define SPI_READ_FREQUENCY 20000000
