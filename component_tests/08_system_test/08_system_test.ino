#include <Wire.h>
#include <Adafruit_AW9523.h>
#include <LedControl.h>
#include <Adafruit_LiquidCrystal.h>

// Whole-system test: every input drives a visible output, so you can check
// everything wired so far at once. Same pins as the game sketch.

// ---- Uno pins ----
const uint8_t DIGIT_PINS[] = {3, 5, 6, 7};
const uint8_t SEG_DATA = 2, SEG_CLOCK = 4, SEG_LATCH = 8;
const uint8_t POT_PINS[] = {A0, A1, A2};
const uint8_t WARNING_PIN = A3;
const bool COMMON_ANODE_DISPLAY = false;  // Part number ends in "BS"? Set true.
const uint8_t MATRIX_DIN = 11, MATRIX_CLK = 13, MATRIX_CS = 10;

// ---- AW9523 pins ----
const uint8_t FLASH_BUTTON = 4, START = 5, RESET = 6, INPUT_COUNT = 10;
const uint8_t TOGGLE_PINS[] = {9, 8, 7};  // Switches 1–3, left to right.
const uint8_t STATUS_FIRST = 10, RGB_FIRST = 13;  // RGB: 13 red, 14 green, 15 blue. LOW lights.
const char *const INPUT_NAMES[INPUT_COUNT] = {
  "Red key", "Yellow key", "Green key", "Blue key",
  "Flash button", "Start", "Reset", "Toggle 3", "Toggle 2", "Toggle 1"
};

Adafruit_AW9523 io;
LedControl matrix(MATRIX_DIN, MATRIX_CLK, MATRIX_CS, 1);
Adafruit_LiquidCrystal lcd(0);
bool ioReady = false;

// ---- Matrix: shows the symbol for the key being pressed ----
const uint8_t SYMBOL_ROWS[4][3] = {
  {0b111, 0b101, 0b111},  // Box   -> red key
  {0b010, 0b111, 0b010},  // Plus  -> yellow key
  {0b101, 0b010, 0b101},  // Cross -> green key
  {0b000, 0b010, 0b000},  // Dot   -> blue key
};

// key -1 shows only the top-left LED.
void showMatrix(int8_t key) {
  matrix.shutdown(0, false);
  matrix.setScanLimit(0, 7);
  matrix.setIntensity(0, 4);
  for (uint8_t row = 0; row < 8; ++row) {
    uint8_t bits = 0;
    if (key < 0) {
      if (row == 0) bits = 0x80;
    } else if (row >= 1 && row <= 6) {
      for (uint8_t x = 0; x < 6; ++x) {
        if (SYMBOL_ROWS[key][(row - 1) / 2] & (4 >> (x / 2))) bits |= 0x80 >> (x + 1);
      }
    }
    matrix.setRow(0, row, bits);  // Bit 7 is the left column.
  }
}

// ---- Countdown display: dial 1, dial 2, dial 3, last pressed AW pin ----
const uint8_t DIGIT_SEGMENTS[] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F};
volatile uint8_t displaySegments[4] = {};

ISR(TIMER2_COMPA_vect) {
  static uint8_t digit = 0;
  digitalWrite(DIGIT_PINS[digit], COMMON_ANODE_DISPLAY ? LOW : HIGH);
  digit = (digit + 1) % 4;
  uint8_t bits = displaySegments[digit];
  digitalWrite(SEG_LATCH, LOW);
  shiftOut(SEG_DATA, SEG_CLOCK, MSBFIRST, COMMON_ANODE_DISPLAY ? (uint8_t)~bits : bits);
  digitalWrite(SEG_LATCH, HIGH);
  digitalWrite(DIGIT_PINS[digit], COMMON_ANODE_DISPLAY ? HIGH : LOW);
}

void startDisplayRefresh() {
  noInterrupts();
  TCCR2A = _BV(WGM21);
  TCCR2B = _BV(CS22) | _BV(CS20);  // 16 MHz / 128.
  OCR2A = 249;                     // 500 Hz digit refresh.
  TIMSK2 = _BV(OCIE2A);
  interrupts();
}

// ---- State ----
uint16_t lastPressedBits = 0xFFFF;  // Forces the first update.
uint8_t lastDials[3] = {255, 255, 255};
int8_t lastPin = -1;

void setup() {
  Serial.begin(115200);
  for (uint8_t i = 0; i < 4; ++i) {
    digitalWrite(DIGIT_PINS[i], COMMON_ANODE_DISPLAY ? LOW : HIGH);
    pinMode(DIGIT_PINS[i], OUTPUT);
  }
  pinMode(SEG_DATA, OUTPUT);
  pinMode(SEG_CLOCK, OUTPUT);
  pinMode(SEG_LATCH, OUTPUT);
  pinMode(WARNING_PIN, OUTPUT);
  matrix.clearDisplay(0);
  showMatrix(-1);

  Wire.begin();
  lcd.begin(16, 2);
  lcd.setBacklight(HIGH);
  ioReady = io.begin(0x58);
  if (ioReady) {
    for (uint8_t i = 0; i < INPUT_COUNT; ++i) io.pinMode(i, INPUT);  // External 10k pull-ups.
    for (uint8_t i = STATUS_FIRST; i < 16; ++i) {
      io.digitalWrite(i, i >= RGB_FIRST ? HIGH : LOW);  // Everything off.
      io.pinMode(i, OUTPUT);
    }
    Serial.println(F("System test. Press keys, buttons, toggles; turn pots."));
  } else {
    lcd.print(F("AW9523 missing"));
    Serial.println(F("AW9523 not found at 0x58. Check power and A4/A5. Pots still work."));
  }
  startDisplayRefresh();
}

void loop() {
  static uint32_t lastScan = 0;
  if (millis() - lastScan < 20) return;
  lastScan = millis();

  // Pots -> first three countdown digits and LCD row 2.
  bool dialsChanged = false;
  for (uint8_t i = 0; i < 3; ++i) {
    uint8_t dial = (uint32_t)analogRead(POT_PINS[i]) * 10 / 1024;
    displaySegments[i] = DIGIT_SEGMENTS[dial];
    if (dial != lastDials[i]) {
      lastDials[i] = dial;
      dialsChanged = true;
      Serial.print(F("Dial "));
      Serial.print(i + 1);
      Serial.print(F(": "));
      Serial.println(dial);
    }
  }

  uint16_t pressedBits = ioReady ? (~io.inputGPIO() & 0x3FF) : 0;  // 1 = pressed / ON.
  bool inputsChanged = pressedBits != lastPressedBits;
  if (inputsChanged) {
    for (uint8_t i = 0; i < INPUT_COUNT; ++i) {
      bool now = pressedBits & (1 << i), before = lastPressedBits & (1 << i);
      if (lastPressedBits == 0xFFFF || now == before) continue;
      if (now) lastPin = i;
      Serial.print(F("AW pin "));
      Serial.print(i);
      Serial.print(F(" ("));
      Serial.print(INPUT_NAMES[i]);
      Serial.println(now ? F("): PRESSED / ON") : F("): released / OFF"));
    }
    lastPressedBits = pressedBits;

    // Matrix: symbol of the lowest pressed colored key.
    int8_t key = -1;
    for (int8_t i = 3; i >= 0; --i) if (pressedBits & (1 << i)) key = i;
    showMatrix(key);

    if (ioReady) {
      bool flash = pressedBits & (1 << FLASH_BUTTON);
      bool red = flash || (pressedBits & 0b0011);    // Red or yellow key.
      bool green = flash || (pressedBits & 0b0110);  // Yellow or green key.
      bool blue = flash || (pressedBits & 0b1000);   // Blue key.
      io.digitalWrite(RGB_FIRST, !red);
      io.digitalWrite(RGB_FIRST + 1, !green);
      io.digitalWrite(RGB_FIRST + 2, !blue);
      bool reset = pressedBits & (1 << RESET);
      for (uint8_t i = 0; i < 3; ++i) {
        io.digitalWrite(STATUS_FIRST + i, reset || (pressedBits & (1 << TOGGLE_PINS[i])));
      }
    }
    digitalWrite(WARNING_PIN, (pressedBits & (1 << START)) ? HIGH : LOW);
    displaySegments[3] = lastPin < 0 ? 0 : DIGIT_SEGMENTS[lastPin];
  }

  if (ioReady && (inputsChanged || dialsChanged)) {
    lcd.setCursor(0, 0);
    lcd.print(F("In:"));
    for (uint8_t i = 0; i < INPUT_COUNT; ++i) lcd.print((pressedBits & (1 << i)) ? '1' : '.');
    lcd.setCursor(0, 1);
    lcd.print(F("D:"));
    for (uint8_t i = 0; i < 3; ++i) { lcd.print(lastDials[i]); lcd.print(' '); }
    lcd.print(F("S:"));
    for (uint8_t i = 0; i < 3; ++i) {
      lcd.print((pressedBits & (1 << TOGGLE_PINS[i])) ? 1 : 0);
      if (i < 2) lcd.print(' ');
    }
  }
}
