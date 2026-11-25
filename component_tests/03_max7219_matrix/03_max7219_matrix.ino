#include <LedControl.h>

const uint8_t MATRIX_DIN = 11;
const uint8_t MATRIX_CLK = 13;
const uint8_t MATRIX_CS = 10;

LedControl matrix(MATRIX_DIN, MATRIX_CLK, MATRIX_CS, 1);

void setup() {
  matrix.shutdown(0, false);
  matrix.setIntensity(0, 4);
  matrix.clearDisplay(0);
}

void loop() {
  matrix.clearDisplay(0);
  matrix.setLed(0, 0, 0, true);  // Find this corner pixel first.
  delay(1000);

  matrix.clearDisplay(0);
  matrix.setLed(0, 0, 0, true);
  matrix.setLed(0, 0, 7, true);
  matrix.setLed(0, 7, 0, true);
  matrix.setLed(0, 7, 7, true);
  delay(1000);

  for (uint8_t row = 0; row < 8; ++row) {
    matrix.setRow(0, row, 0xFF);
    delay(150);
  }
  delay(400);

  matrix.clearDisplay(0);
  for (uint8_t row = 0; row < 8; ++row) {
    for (uint8_t column = 0; column < 8; ++column) {
      matrix.setLed(0, row, column, true);
      delay(35);
    }
  }
  matrix.clearDisplay(0);
  delay(500);
}
