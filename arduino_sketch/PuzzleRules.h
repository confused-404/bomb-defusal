#pragma once
#include <stdint.h>

// Button indices: red=0, yellow=1, green=2, blue=3.
inline uint8_t makeSymbolAnswer(uint8_t level, const uint8_t *s, uint8_t *answer) {
  if (level == 0) { answer[0] = s[0]; return 1; }
  if (level == 1) { answer[0] = s[1]; answer[1] = s[0]; return 2; }
  for (uint8_t i = 0; i < 4; ++i) {
    answer[2 * i] = s[i];
    answer[2 * i + 1] = (s[i] + 2) % 4;
  }
  return 8;
}

// Flash colors: red=0, green=1, blue=2. Dot=1 unit, dash=2 units.
inline uint8_t makeFlashAnswer(uint8_t count, const uint8_t *colors, const uint8_t *longFlash) {
  uint8_t taps = 0;
  for (uint8_t i = 0; i < count; ++i) taps += (colors[i] + 1) * (longFlash[i] ? 2 : 1);
  return taps;
}

inline void makeSerialAnswer(uint8_t level, const uint8_t *code, uint8_t *dials, uint8_t *switches) {
  for (uint8_t i = 0; i < 3; ++i) {
    dials[i] = level == 0 ? code[i] :
               (level == 1 ? (code[i] + code[i + 3]) % 10 : (code[i] ^ code[i + 3]) % 10);
    switches[i] = level == 2 ? ((code[i + 3] & 1) ^ (code[0] >= 8)) : 0;
  }
  if (level == 1) switches[0] = (code[0] + code[1] + code[2]) % 2;
}

// Converts a pot reading (0-1023) to a digit 0-9 using ten equal ranges.
// A small dead zone at each boundary stops the value flickering between two digits.
inline uint8_t readPotDigit(uint16_t adc, uint8_t previous) {
  uint8_t digit = (uint32_t)adc * 10 / 1024;
  if (digit > previous && adc < ((uint16_t)(previous + 1) * 1024 + 9) / 10 + 8) return previous;
  if (digit < previous && adc + 8 >= ((uint16_t)previous * 1024 + 9) / 10) return previous;
  return digit;
}
