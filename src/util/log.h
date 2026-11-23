#pragma once

#include <Arduino.h>

// Log simples sobre Serial. LOG_LEVEL: 0=erro, 1=+info, 2=+debug.
// As medições por página/troca (Grade, Ficha, Vitrine: tempo e maior bloco
// livre) são debug: build_flags = -D LOG_LEVEL=2 pra ver.
#ifndef LOG_LEVEL
#define LOG_LEVEL 1
#endif

#define LOG_E(tag, fmt, ...) Serial.printf("[E][%s] " fmt "\n", tag, ##__VA_ARGS__)

#if LOG_LEVEL >= 1
#define LOG_I(tag, fmt, ...) Serial.printf("[I][%s] " fmt "\n", tag, ##__VA_ARGS__)
#else
#define LOG_I(tag, fmt, ...)
#endif

#if LOG_LEVEL >= 2
#define LOG_D(tag, fmt, ...) Serial.printf("[D][%s] " fmt "\n", tag, ##__VA_ARGS__)
#else
#define LOG_D(tag, fmt, ...)
#endif
