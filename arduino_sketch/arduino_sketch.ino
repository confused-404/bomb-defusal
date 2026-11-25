#include <Wire.h>
#include <Adafruit_AW9523.h>
#include <LedControl.h>
#include <Adafruit_LiquidCrystal.h>
#include "PuzzleRules.h"

// Wiring and the matching expert manual are in ../planning/WIRING.md.

// ---- Uno pins ----
const uint8_t DIGIT_PINS[] = {3, 5, 6, 7};                 // Countdown digits 1-4, left to right.
const uint8_t SEG_DATA = 2, SEG_CLOCK = 4, SEG_LATCH = 8;  // 74HC595.
const uint8_t POT_PINS[] = {A0, A1, A2};                   // Serial Code dials 1-3.
const uint8_t WARNING_PIN = A3;                            // Red low-time LED.
const bool COMMON_ANODE_DISPLAY = false;  // Part number ends in "BS"? Set true.
const uint8_t MATRIX_DIN = 11, MATRIX_CLK = 13, MATRIX_CS = 10;  // MAX7219 module.
const uint8_t MATRIX_COUNT = 1;     // Set 4 for a 4-in-1 module; the puzzle uses the first matrix.
const uint8_t MATRIX_ROTATION = 0;  // 0-3 quarter turns clockwise. See WIRING.md orientation check.
const bool MATRIX_MIRROR = false;
// D0, D1, D9, and D12 are unused.

// ---- AW9523 pins ----
// Inputs 0-3: red, yellow, green, blue symbol keys. 7-9: Serial Code toggles 1-3.
const uint8_t LARGE = 4, START = 5, RESET = 6, TOGGLE_FIRST = 7, INPUT_COUNT = 10;
const uint8_t STATUS_FIRST = 10;  // Green LEDs 10-12: Symbol, Flash, Serial.
const uint8_t RGB_FIRST = 13;     // 13 red, 14 green, 15 blue. LOW lights.

const uint16_t ROUND_SECONDS[] = {180, 300, 480};
const uint32_t DEBOUNCE_MS = 30, TAP_GAP_MS = 1500, SERIAL_HOLD_MS = 750;

Adafruit_AW9523 io;
LedControl matrix(MATRIX_DIN, MATRIX_CLK, MATRIX_CS, MATRIX_COUNT);
Adafruit_LiquidCrystal lcd(0);   // Adafruit #292 backpack at I2C address 0x20.

enum GameState { SETUP, PLAYING, WON, LOST };
GameState state = SETUP;
uint8_t difficulty = 0, timerChoice = 1;
bool solved[3] = {};
uint32_t remainingMs = 0, lastTick = 0, lastScan = 0;
uint16_t lastShownSeconds = 65535;
uint8_t strikes = 0;

bool rawInput[INPUT_COUNT] = {}, stableInput[INPUT_COUNT] = {};
bool pressed[INPUT_COUNT] = {}, released[INPUT_COUNT] = {};
uint32_t changedAt[INPUT_COUNT] = {};

uint8_t symbols[4] = {}, symbolAnswer[8] = {}, symbolLength = 0, symbolPosition = 0;

uint8_t flashColors[5] = {}, flashLong[5] = {}, flashLength = 0, flashAnswer = 0;
uint8_t flashIndex = 0, tapCount = 0;
bool flashOn = false;
uint32_t flashChanged = 0, lastTapRelease = 0;

// Serial Code: three pots (dials 0-9) and three toggles (0 = OFF, 1 = ON).
uint8_t serialDigits[6] = {}, dialAnswer[3] = {}, switchAnswer[3] = {};
uint8_t dials[3] = {}, switches[3] = {};
bool serialMatching = false;
uint32_t serialMatchedAt = 0;

// ---- 8x8 LED matrix (MAX7219) for the Symbol Sequence ----
// frame[y] bit x = LED at column x, row y, with (0, 0) the top-left as mounted.
uint8_t frame[8] = {};

// 3x3 symbols, one byte per row, bit 2 = left column.
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

// Draws a symbol with its top-left corner at (x0, y0); scale 2 doubles its size.
void drawSymbol(uint8_t symbol, uint8_t x0, uint8_t y0, uint8_t scale) {
  for (uint8_t y = 0; y < 3 * scale; ++y)
    for (uint8_t x = 0; x < 3 * scale; ++x)
      if (SYMBOL_ROWS[symbol][y / scale] & (4 >> (x / scale))) frame[y0 + y] |= 1 << (x0 + x);
}

// Sends the frame to the module, turned to match how the module is mounted.
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
      rows[r] |= 0x80 >> c;  // LedControl: bit 7 is column 0.
    }
  }
  for (uint8_t r = 0; r < 8; ++r) matrix.setRow(0, r, rows[r]);
}

// Easy: one big symbol. Medium: left and right. Hard: four corners.
void drawSymbolPuzzle() {
  clearFrame();
  if (difficulty == 0) drawSymbol(symbols[0], 1, 1, 2);
  else if (difficulty == 1) {
    drawSymbol(symbols[0], 0, 2, 1);
    drawSymbol(symbols[1], 5, 2, 1);
  } else {
    drawSymbol(symbols[0], 0, 0, 1);  // Top-left
    drawSymbol(symbols[1], 5, 0, 1);  // Top-right
    drawSymbol(symbols[2], 0, 5, 1);  // Bottom-left
    drawSymbol(symbols[3], 5, 5, 1);  // Bottom-right
  }
  showFrame();
}
// ---- Countdown display (multiplexed by a timer interrupt) ----
// Bit order matches 74HC595 Q0-Q7 -> segments A B C D E F G DP.
const uint8_t DIGIT_SEGMENTS[] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F};
const uint8_t DECIMAL_POINT = 0x80;
volatile uint8_t displaySegments[4] = {};

// Lights one digit at a time, 500 times a second. Running from an interrupt keeps
// the display steady while the main loop is busy with I2C.
ISR(TIMER2_COMPA_vect) {
  static uint8_t digit = 0;
  digitalWrite(DIGIT_PINS[digit], COMMON_ANODE_DISPLAY ? LOW : HIGH);  // Off.
  digit = (digit + 1) % 4;
  uint8_t bits = displaySegments[digit];
  digitalWrite(SEG_LATCH, LOW);
  shiftOut(SEG_DATA, SEG_CLOCK, MSBFIRST, COMMON_ANODE_DISPLAY ? (uint8_t)~bits : bits);
  digitalWrite(SEG_LATCH, HIGH);
  digitalWrite(DIGIT_PINS[digit], COMMON_ANODE_DISPLAY ? HIGH : LOW);  // On.
}

void startDisplayRefresh() {
  noInterrupts();
  TCCR2A = _BV(WGM21);             // CTC mode.
  TCCR2B = _BV(CS22) | _BV(CS20);  // 16 MHz / 128.
  OCR2A = 249;                     // / 250 = 500 Hz.
  TIMSK2 = _BV(OCIE2A);
  interrupts();
}

void showTime(uint16_t seconds) {
  if (seconds == lastShownSeconds) return;
  lastShownSeconds = seconds;
  uint8_t minutes = seconds / 60, secs = seconds % 60;
  displaySegments[0] = DIGIT_SEGMENTS[minutes / 10];
  displaySegments[1] = DIGIT_SEGMENTS[minutes % 10] | DECIMAL_POINT;  // Shows MM.SS
  displaySegments[2] = DIGIT_SEGMENTS[secs / 10];
  displaySegments[3] = DIGIT_SEGMENTS[secs % 10];
}

// ---- Small helpers ----
void rgbOff() {
  for (uint8_t i = 0; i < 3; ++i) io.digitalWrite(RGB_FIRST + i, HIGH);
}

void rgbOn(uint8_t color) { io.digitalWrite(RGB_FIRST + color, LOW); }

// LCD row 2 shows the live readings, e.g. "D:4 0 7 S:0 1 0", so faceplate
// markings don't have to be perfectly calibrated.
void drawSerialControls() {
  lcd.setCursor(0, 1);
  lcd.print(F("D:"));
  for (uint8_t i = 0; i < 3; ++i) { lcd.print(dials[i]); lcd.print(' '); }
  lcd.print(F("S:"));
  for (uint8_t i = 0; i < 3; ++i) { lcd.print(switches[i]); if (i < 2) lcd.print(' '); }
}

// Returns true if any dial or switch changed.
bool readSerialControls() {
  bool changed = false;
  for (uint8_t i = 0; i < 3; ++i) {
    uint8_t dial = readPotDigit(analogRead(POT_PINS[i]), dials[i]);
    uint8_t toggle = stableInput[TOGGLE_FIRST + i];  // Closed to GND = ON.
    if (dial != dials[i] || toggle != switches[i]) changed = true;
    dials[i] = dial;
    switches[i] = toggle;
  }
  return changed;
}

bool serialSettingsMatch() {
  for (uint8_t i = 0; i < 3; ++i) {
    if (dials[i] != dialAnswer[i]) return false;
    // Easy ignores switches; Medium uses only switch 1; Hard uses all three.
    bool switchUsed = difficulty == 2 || (difficulty == 1 && i == 0);
    if (switchUsed && switches[i] != switchAnswer[i]) return false;
  }
  return true;
}

// ---- Screens and game state ----
void showSetup() {
  clearFrame();
  frame[0] = 1;  // Top-left LED: orientation marker.
  showFrame();
  lcd.clear();
  lcd.print(F("Level: "));
  if (difficulty == 0) lcd.print(F("Easy"));
  else if (difficulty == 1) lcd.print(F("Medium"));
  else lcd.print(F("Hard"));
  lcd.setCursor(0, 1);
  lcd.print(F("B=time START=go"));
  showTime(ROUND_SECONDS[timerChoice]);
}

void resetGame() {
  state = SETUP;
  for (uint8_t i = 0; i < 3; ++i) {
    solved[i] = false;
    io.digitalWrite(STATUS_FIRST + i, LOW);
  }
  digitalWrite(WARNING_PIN, LOW);
  rgbOff();
  tapCount = 0;
  symbolPosition = 0;
  serialMatching = false;
  strikes = 0;
  showSetup();  // Keeps the chosen difficulty and timer.
}

void finishGame(bool win) {
  state = win ? WON : LOST;
  rgbOff();
  digitalWrite(WARNING_PIN, !win);
  for (uint8_t y = 0; y < 8; ++y) frame[y] = win ? CHECK_MARK[y] : (1 << y) | (0x80 >> y);  // Check or big X.
  showFrame();
  lcd.clear();
  if (win) lcd.print(F("DEFUSED!"));
  else lcd.print(F("BOOM! Time's up"));
  lcd.setCursor(0, 1);
  lcd.print(F("RESET to retry"));
}

// Called before accepting actions, so an action at zero cannot win.
void updateClock(uint32_t now) {
  if (state != PLAYING) return;
  uint32_t elapsed = now - lastTick;
  lastTick = now;
  if (elapsed >= remainingMs) {
    remainingMs = 0;
    showTime(0);
    finishGame(false);
    return;
  }
  remainingMs -= elapsed;
  showTime((remainingMs + 999) / 1000);
}

void penalty() {
  updateClock(millis());
  if (state != PLAYING) return;
  if (strikes < 99) ++strikes;
  remainingMs = remainingMs > 10000 ? remainingMs - 10000 : 0;
  showTime((remainingMs + 999) / 1000);
  lcd.setCursor(12, 0);
  lcd.print(F("X:"));
  lcd.print(strikes);
  if (!remainingMs) finishGame(false);
}

void completeModule(uint8_t module) {
  updateClock(millis());
  if (state != PLAYING) return;
  solved[module] = true;
  io.digitalWrite(STATUS_FIRST + module, HIGH);
  if (module == 1) rgbOff();
  if (solved[0] && solved[1] && solved[2]) finishGame(true);
}

void startGame() {
  randomSeed(micros());  // Human press timing makes each round different.
  for (uint8_t i = 0; i < 4; ++i) symbols[i] = random(4);
  symbolLength = makeSymbolAnswer(difficulty, symbols, symbolAnswer);
  symbolPosition = 0;

  flashLength = difficulty == 0 ? 1 : (difficulty == 1 ? 3 : 5);
  uint8_t firstColor = random(3);
  for (uint8_t i = 0; i < flashLength; ++i) {
    flashColors[i] = difficulty == 0 ? 0 : (difficulty == 1 ? (firstColor + i) % 3 : random(3));
    flashLong[i] = random(2);
  }
  flashAnswer = makeFlashAnswer(flashLength, flashColors, flashLong);

  // Pick a code whose answer differs from the current knob/switch positions,
  // so the module never solves itself.
  readSerialControls();
  do {
    for (uint8_t i = 0; i < 6; ++i) serialDigits[i] = random(difficulty == 2 ? 16 : 10);
    makeSerialAnswer(difficulty, serialDigits, dialAnswer, switchAnswer);
  } while (serialSettingsMatch());
  serialMatching = false;

  drawSymbolPuzzle();

  lcd.clear();
  lcd.print(F("Code:"));
  for (uint8_t i = 0; i < (difficulty == 0 ? 3 : 6); ++i) {
    lcd.print((char)(serialDigits[i] < 10 ? '0' + serialDigits[i] : 'A' + serialDigits[i] - 10));
  }
  lcd.setCursor(12, 0);
  lcd.print(F("X:0"));
  drawSerialControls();

  tapCount = 0;
  flashIndex = 0;
  flashOn = true;
  rgbOff();
  rgbOn(flashColors[0]);
  uint32_t now = millis();
  flashChanged = lastTick = now;
  remainingMs = (uint32_t)ROUND_SECONDS[timerChoice] * 1000;
  state = PLAYING;
  showTime(ROUND_SECONDS[timerChoice]);
}

// ---- Inputs ----
void scanInputs(uint32_t now) {
  uint16_t bits = io.inputGPIO();
  for (uint8_t i = 0; i < INPUT_COUNT; ++i) {
    pressed[i] = released[i] = false;
    bool down = !(bits & ((uint16_t)1 << i));  // Pressed / ON pulls the pin LOW.
    if (down != rawInput[i]) {
      rawInput[i] = down;
      changedAt[i] = now;
    }
    if (now - changedAt[i] >= DEBOUNCE_MS && stableInput[i] != down) {
      stableInput[i] = down;
      pressed[i] = down;
      released[i] = !down;
    }
  }
}

// ---- Modules ----
void updateFlashes(uint32_t now) {
  if (solved[1]) return;
  uint32_t duration = flashOn ? (flashLong[flashIndex] ? 750 : 250)
                              : (flashIndex == flashLength - 1 ? 2000 : 250);
  if (now - flashChanged < duration) return;
  flashChanged = now;
  if (flashOn) {
    rgbOff();
    flashOn = false;
  } else {
    flashIndex = (flashIndex + 1) % flashLength;
    rgbOn(flashColors[flashIndex]);
    flashOn = true;
  }
}

void updateSerial(uint32_t now) {
  if (solved[2]) return;
  if (readSerialControls()) drawSerialControls();
  if (!serialSettingsMatch()) serialMatching = false;
  else if (!serialMatching) {
    serialMatching = true;
    serialMatchedAt = now;
  } else if (now - serialMatchedAt >= SERIAL_HOLD_MS) completeModule(2);
}

// ---- Arduino entry points ----
void setup() {
  for (uint8_t i = 0; i < 4; ++i) {
    digitalWrite(DIGIT_PINS[i], COMMON_ANODE_DISPLAY ? LOW : HIGH);  // Start dark.
    pinMode(DIGIT_PINS[i], OUTPUT);
  }
  pinMode(SEG_DATA, OUTPUT);
  pinMode(SEG_CLOCK, OUTPUT);
  pinMode(SEG_LATCH, OUTPUT);
  pinMode(WARNING_PIN, OUTPUT);
  for (uint8_t i = 0; i < 3; ++i) dials[i] = (uint32_t)analogRead(POT_PINS[i]) * 10 / 1024;

  for (uint8_t i = 0; i < MATRIX_COUNT; ++i) {
    matrix.shutdown(i, false);  // Wake up.
    matrix.setIntensity(i, 4);  // 0-15. Low keeps battery draw down.
    matrix.clearDisplay(i);
  }

  Wire.begin();
  lcd.begin(16, 2);
  lcd.setBacklight(HIGH);
  if (!io.begin(0x58)) {
    lcd.print(F("AW9523 missing:"));
    lcd.setCursor(0, 1);
    lcd.print(F("check I2C wires"));
    while (true) delay(100);
  }
  for (uint8_t i = 0; i < INPUT_COUNT; ++i) io.pinMode(i, INPUT);  // External 10k pull-ups.
  for (uint8_t i = STATUS_FIRST; i < STATUS_FIRST + 3; ++i) {
    io.digitalWrite(i, LOW);
    io.pinMode(i, OUTPUT);
  }
  for (uint8_t i = RGB_FIRST; i < RGB_FIRST + 3; ++i) {
    io.digitalWrite(i, HIGH);  // RGB off.
    io.pinMode(i, OUTPUT);
  }
  uint16_t bits = io.inputGPIO();
  for (uint8_t i = 0; i < INPUT_COUNT; ++i) rawInput[i] = stableInput[i] = !(bits & ((uint16_t)1 << i));
  startDisplayRefresh();
  resetGame();
}

void loop() {
  // Poll inputs every 5ms.
  uint32_t now = millis();
  updateClock(now);
  if (now - lastScan < 5) return;
  lastScan = now;
  scanInputs(now);
  if (pressed[RESET]) {
    resetGame();
    return;
  }
  if (state == SETUP) {
    bool changed = false;
    for (uint8_t i = 0; i < 3; ++i) {
      if (pressed[i]) { difficulty = i; changed = true; }
    }
    if (pressed[3]) { timerChoice = (timerChoice + 1) % 3; changed = true; }
    if (changed) showSetup();
    if (pressed[START]) startGame();
    return;  // Setup presses never count as puzzle answers.
  }
  if (state != PLAYING) return;

  // Symbol Sequence
  if (!solved[0]) {
    for (uint8_t i = 0; i < 4 && state == PLAYING; ++i) {
      if (!pressed[i]) continue;
      if (i == symbolAnswer[symbolPosition]) {
        if (++symbolPosition == symbolLength) { completeModule(0); break; }
      } else {
        symbolPosition = 0;
        penalty();
      }
    }
  }
  if (state != PLAYING) return;

  // RGB Flash Sequence: count taps, submit after a quiet gap.
  if (!solved[1]) {
    if (pressed[LARGE] && tapCount < 255) ++tapCount;
    if (released[LARGE]) lastTapRelease = now;
    if (tapCount && !stableInput[LARGE] && now - lastTapRelease >= TAP_GAP_MS) {
      bool correct = tapCount == flashAnswer;
      tapCount = 0;
      if (correct) completeModule(1);
      else penalty();
    }
  }
  if (state != PLAYING) return;
  updateFlashes(now);

  // Serial Code
  updateSerial(now);
  if (state != PLAYING) return;

  bool lowTime = remainingMs <= 30000;
  digitalWrite(WARNING_PIN, lowTime && ((now / 250) % 2));
}
