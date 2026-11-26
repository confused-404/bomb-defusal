#include <LedControl.h>

// Previews the puzzle and result shapes, plus a static orientation marker.
const uint8_t MATRIX_DIN = 11, MATRIX_CLK = 13, MATRIX_CS = 10;
const uint8_t MATRIX_ROTATION = 0;  // Keep in sync with arduino_sketch.ino.
const bool MATRIX_MIRROR = false;

LedControl matrix(MATRIX_DIN, MATRIX_CLK, MATRIX_CS, 1);

// frame[y] bit x = LED at column x, row y, with (0, 0) the top-left as mounted.
uint8_t frame[8] = {};

const uint8_t SYMBOL_ROWS[4][3] = {
  {0b111, 0b101, 0b111},  // 0 Box   -> red key
  {0b010, 0b111, 0b010},  // 1 Plus  -> yellow key
  {0b101, 0b010, 0b101},  // 2 Cross -> green key
  {0b000, 0b010, 0b000},  // 3 Dot   -> blue key
};
const uint8_t CHECK_MARK[8] = {0x00, 0x80, 0x40, 0x20, 0x11, 0x0A, 0x04, 0x00};

void clearFrame() {
  for (uint8_t y = 0; y < 8; ++y) frame[y] = 0;
}

void drawSymbol(uint8_t symbol, uint8_t x0, uint8_t y0, uint8_t scale) {
  for (uint8_t y = 0; y < 3 * scale; ++y)
    for (uint8_t x = 0; x < 3 * scale; ++x)
      if (SYMBOL_ROWS[symbol][y / scale] & (4 >> (x / scale))) frame[y0 + y] |= 1 << (x0 + x);
}

void showFrame() {
  uint8_t rows[8] = {};
  for (uint8_t y = 0; y < 8; ++y) {
    for (uint8_t x = 0; x < 8; ++x) {
      if (!(frame[y] & (1 << x))) continue;
      uint8_t px = MATRIX_MIRROR ? 7 - x : x, r, c;
      switch (MATRIX_ROTATION) {
        case 1: r = px; c = 7 - y; break;
        case 2: r = 7 - y; c = 7 - px; break;
        case 3: r = 7 - px; c = y; break;
        default: r = y; c = px; break;
      }
      rows[r] |= 0x80 >> c;
    }
  }
  matrix.shutdown(0, false);
  matrix.setScanLimit(0, 7);
  matrix.setIntensity(0, 4);
  for (uint8_t r = 0; r < 8; ++r) matrix.setRow(0, r, rows[r]);
}

void hold(uint16_t ms) {
  showFrame();
  delay(ms);
}

void setup() {
  matrix.clearDisplay(0);
}

void loop() {
  // 1. Setup screen: top-left LED only.
  clearFrame();
  frame[0] = 1;
  hold(2000);

  // 2. Easy: each symbol big, in key order Box, Plus, Cross, Dot.
  for (uint8_t s = 0; s < 4; ++s) {
    clearFrame();
    drawSymbol(s, 1, 1, 2);
    hold(1500);
  }

  // 3. Medium and Hard: Box top-left, Plus top-right, Cross bottom-left, Dot bottom-right.
  clearFrame();
  drawSymbol(0, 0, 0, 1);
  drawSymbol(1, 5, 0, 1);
  drawSymbol(2, 0, 5, 1);
  drawSymbol(3, 5, 5, 1);
  hold(3000);

  // 4. Win: check mark. 5. Loss: big X.
  for (uint8_t y = 0; y < 8; ++y) frame[y] = CHECK_MARK[y];
  hold(2000);
  for (uint8_t y = 0; y < 8; ++y) frame[y] = (1 << y) | (0x80 >> y);
  hold(2000);
}
