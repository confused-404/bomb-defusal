#include <LedControl.h>

const uint8_t MATRIX_DIN = 11;
const uint8_t MATRIX_CLK = 13;
const uint8_t MATRIX_CS = 10;

LedControl matrix(MATRIX_DIN, MATRIX_CLK, MATRIX_CS, 1);

// Re-sent every loop so a glitch on a long or loose wire can't leave the module stuck.
void configureMatrix() {
  matrix.shutdown(0, false);
  matrix.setScanLimit(0, 7);
  matrix.setIntensity(0, 4);
}

void setup() {
  configureMatrix();
  matrix.clearDisplay(0);
}

void loop() {
  configureMatrix();
  for (uint8_t row = 0; row < 8; ++row) matrix.setRow(0, row, 0xFF);  // All 64 LEDs.
  delay(2000);

  configureMatrix();
  for (uint8_t row = 0; row < 8; ++row) {
    matrix.setRow(0, row, (row == 0 || row == 7) ? 0xFF : 0x81);  // Outer ring only.
  }
  delay(2000);
}
