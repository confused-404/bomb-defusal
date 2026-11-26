#include <LedControl.h>

const uint8_t MATRIX_DIN = 11;
const uint8_t MATRIX_CLK = 13;
const uint8_t MATRIX_CS = 10;

LedControl matrix(MATRIX_DIN, MATRIX_CLK, MATRIX_CS, 1);

void configureMatrix() {
  matrix.shutdown(0, false);
  matrix.setScanLimit(0, 7);
  matrix.setIntensity(0, 4);
}

void setup() {
  configureMatrix();
  matrix.clearDisplay(0);
}

// Fills row 0 one LED at a time, starting at column 0, then holds and repeats.
// Where it starts and which way it runs shows how the game's top-left maps to the module.
void loop() {
  configureMatrix();
  matrix.clearDisplay(0);
  delay(1000);
  for (uint8_t column = 0; column < 8; ++column) {
    matrix.setLed(0, 0, column, true);
    delay(400);
  }
  delay(2000);
}
