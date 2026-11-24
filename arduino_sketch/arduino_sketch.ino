#include <Wire.h>
#include <SPI.h>
#include <Adafruit_AW9523.h>
#include <Adafruit_ST7789.h>
#include <Adafruit_LEDBackpack.h>
#include <Adafruit_LiquidCrystal.h>
#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>
#include "PuzzleRules.h"

// Wiring and the matching expert manual are in ../WIRING.md.
const uint8_t RGB_PINS[] = {3, 5, 6}; // Common-anode: LOW lights a channel.
const uint8_t POT_PINS[] = {A0, A1, A2};
const uint8_t STATUS_FIRST = 10, WARNING_PIN = 13; // AW9523 pins.
const uint8_t LARGE = 4, START = 5, RESET = 6;     // AW9523 inputs.
const uint16_t ROUND_SECONDS[] = {180, 300, 480};
const uint32_t DEBOUNCE_MS = 30, TAP_GAP_MS = 1500, SERIAL_HOLD_MS = 750;

Adafruit_AW9523 io;
Adafruit_ST7789 tft(10, 9, 8);
Adafruit_7segment timerDisplay;
Adafruit_LiquidCrystal lcd(0); // Adafruit #292, address 0x20, NOT PCF8574.
SoftwareSerial audioSerial(2, 4); // Arduino RX, TX.
DFRobotDFPlayerMini audio;

enum GameState { SETUP, PLAYING, WON, LOST };
GameState state = SETUP;
uint8_t difficulty = 0, timerChoice = 1;
bool audioReady = false, solved[3] = {};
uint32_t remainingMs = 0, lastTick = 0, lastScan = 0, lastWarning = 0;
uint16_t lastShownSeconds = 65535;
uint8_t strikes = 0;

// Debounce all seven buttons and three toggles, including held buttons at boot.
bool rawInput[10] = {}, stableInput[10] = {}, pressed[10] = {}, released[10] = {};
uint32_t changedAt[10] = {};
uint8_t symbols[4] = {}, symbolAnswer[8] = {}, symbolLength = 0, symbolPosition = 0;
uint8_t flashColors[5] = {}, flashLong[5] = {}, flashLength = 0, flashAnswer = 0;
uint8_t flashIndex = 0, tapCount = 0;
bool flashOn = false;
uint32_t flashChanged = 0, lastTapRelease = 0;
uint8_t serialDigits[6] = {}, potAnswer[3] = {}, switchAnswer[3] = {};
uint8_t potValues[3] = {};
bool serialMatching = false;
uint32_t serialMatchedAt = 0;

void sound(uint8_t track) {
  if (audioReady) audio.playMp3Folder(track);
}

void rgbOff() {
  for (uint8_t i = 0; i < 3; ++i) digitalWrite(RGB_PINS[i], HIGH);
}

void showTime(uint16_t seconds) {
  if (seconds == lastShownSeconds) return;
  lastShownSeconds = seconds;
  // Explicit positions preserve leading zeroes: MM:SS.
  timerDisplay.writeDigitNum(0, seconds / 60 / 10);
  timerDisplay.writeDigitNum(1, seconds / 60 % 10);
  timerDisplay.writeDigitNum(3, seconds % 60 / 10);
  timerDisplay.writeDigitNum(4, seconds % 10);
  timerDisplay.drawColon(true);
  timerDisplay.writeDisplay();
}

void showSetup() {
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(8, 8);
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_WHITE);
  tft.println(F("SETUP"));
  tft.println(F("Red: Easy"));
  tft.println(F("Yellow: Medium"));
  tft.println(F("Green: Hard"));
  tft.println(F("Blue: 3/5/8 minutes"));
  tft.print(F("Level: "));
  tft.println(difficulty + 1);
  tft.println(F("Press START"));
  lcd.clear();
  lcd.print(F("Ready: level "));
  lcd.print(difficulty + 1);
  lcd.setCursor(0, 1);
  lcd.print(F("START to play"));
  showTime(ROUND_SECONDS[timerChoice]);
}

void resetGame() {
  state = SETUP;
  for (uint8_t i = 0; i < 3; ++i) {
    solved[i] = false;
    io.digitalWrite(STATUS_FIRST + i, LOW);
  }
  io.digitalWrite(WARNING_PIN, LOW);
  rgbOff();
  tapCount = 0;
  symbolPosition = 0;
  serialMatching = false;
  strikes = 0;
  if (audioReady) audio.stop();
  showSetup(); // Retains the chosen difficulty and timer.
}

void finishGame(bool win) {
  state = win ? WON : LOST;
  rgbOff();
  io.digitalWrite(WARNING_PIN, !win);
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(20, 80);
  tft.setTextSize(3);
  tft.setTextColor(win ? ST77XX_GREEN : ST77XX_RED);
  if (win) tft.println(F("DEFUSED!"));
  else tft.println(F("BOOM!"));
  tft.setTextSize(2);
  tft.println(F("Press RESET"));
  lcd.clear();
  if (win) lcd.print(F("All modules done"));
  else lcd.print(F("Time expired"));
  lcd.setCursor(0, 1);
  lcd.print(F("RESET to retry"));
  sound(win ? 5 : 6);
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
  if (strikes < 255) ++strikes;
  remainingMs = remainingMs > 10000 ? remainingMs - 10000 : 0;
  showTime((remainingMs + 999) / 1000);
  lcd.setCursor(0, 1);
  lcd.print(F("Strikes:        "));
  lcd.setCursor(9, 1);
  lcd.print(strikes);
  if (!remainingMs) finishGame(false);
  else sound(3);
}

void completeModule(uint8_t module) {
  updateClock(millis());
  if (state != PLAYING) return;
  solved[module] = true;
  io.digitalWrite(STATUS_FIRST + module, HIGH);
  if (module == 1) rgbOff();
  if (solved[0] && solved[1] && solved[2]) finishGame(true);
  else sound(4);
}

void drawSymbol(uint8_t symbol, int16_t x, int16_t y) {
  switch (symbol) {
    case 0: tft.drawCircle(x, y, 24, ST77XX_WHITE); break;
    case 1: tft.drawTriangle(x, y - 24, x - 24, y + 24, x + 24, y + 24, ST77XX_WHITE); break;
    case 2: tft.drawRect(x - 24, y - 24, 48, 48, ST77XX_WHITE); break;
    case 3:
      tft.drawLine(x - 24, y, x + 24, y, ST77XX_WHITE);
      tft.drawLine(x, y - 24, x, y + 24, ST77XX_WHITE);
      break;
  }
}

void startGame() {
  // Human start timing and the unused A3 provide a non-security random seed.
  randomSeed(micros() ^ analogRead(A3));
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
  for (uint8_t i = 0; i < 6; ++i) serialDigits[i] = random(difficulty == 2 ? 16 : 10);
  makeSerialAnswer(difficulty, serialDigits, potAnswer, switchAnswer);

  tft.fillScreen(ST77XX_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(8, 8);
  tft.println(F("SYMBOL SEQUENCE"));
  if (difficulty == 0) drawSymbol(symbols[0], 160, 120);
  else if (difficulty == 1) {
    drawSymbol(symbols[0], 90, 120);
    drawSymbol(symbols[1], 230, 120);
  } else {
    tft.drawLine(160, 40, 160, 230, ST77XX_BLUE);
    tft.drawLine(0, 135, 319, 135, ST77XX_BLUE);
    for (uint8_t i = 0; i < 4; ++i) drawSymbol(symbols[i], 80 + (i % 2) * 160, 85 + (i / 2) * 100);
  }
  lcd.clear();
  lcd.print(F("Code: "));
  for (uint8_t i = 0; i < (difficulty == 0 ? 3 : 6); ++i) {
    lcd.print((char)(serialDigits[i] < 10 ? '0' + serialDigits[i] : 'A' + serialDigits[i] - 10));
  }
  lcd.setCursor(0, 1);
  lcd.print(F("Strikes: 0"));

  tapCount = 0;
  serialMatching = false;
  flashIndex = 0;
  flashOn = true;
  rgbOff();
  digitalWrite(RGB_PINS[flashColors[0]], LOW);
  uint32_t now = millis();
  flashChanged = lastTick = lastWarning = now;
  remainingMs = (uint32_t)ROUND_SECONDS[timerChoice] * 1000;
  state = PLAYING;
  showTime(ROUND_SECONDS[timerChoice]);
  sound(1);
}

void scanInputs(uint32_t now) {
  uint16_t bits = io.inputGPIO();
  for (uint8_t i = 0; i < 10; ++i) {
    pressed[i] = released[i] = false;
    bool down = !(bits & ((uint16_t)1 << i));
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
    digitalWrite(RGB_PINS[flashColors[flashIndex]], LOW);
    flashOn = true;
  }
}

void checkSerial(uint32_t now) {
  if (solved[2]) return;
  bool match = true;
  for (uint8_t i = 0; i < 3; ++i) {
    potValues[i] = readPotDigit(analogRead(POT_PINS[i]), potValues[i]);
    if (potValues[i] != potAnswer[i]) match = false;
    // Easy ignores switches; Medium uses only switch 1.
    if ((difficulty == 2 || (difficulty == 1 && i == 0)) &&
        stableInput[7 + i] != (switchAnswer[i] != 0)) match = false;
  }
  if (!match) serialMatching = false;
  else if (!serialMatching) {
    serialMatching = true;
    serialMatchedAt = now;
  } else if (now - serialMatchedAt >= SERIAL_HOLD_MS) completeModule(2);
}

void setup() {
  Serial.begin(115200);
  for (uint8_t i = 0; i < 3; ++i) {
    digitalWrite(RGB_PINS[i], HIGH);
    pinMode(RGB_PINS[i], OUTPUT);
    potValues[i] = (uint32_t)analogRead(POT_PINS[i]) * 10 / 1024;
  }
  Wire.begin();
  tft.init(240, 320);
  tft.setRotation(1);
  if (!io.begin(0x58)) {
    Serial.println(F("AW9523 missing: check power, SDA, SCL."));
    tft.fillScreen(ST77XX_BLACK);
    tft.setCursor(8, 8);
    tft.setTextColor(ST77XX_RED);
    tft.setTextSize(2);
    tft.println(F("AW9523 missing"));
    while (true) delay(100);
  }
  for (uint8_t i = 0; i < 10; ++i) io.pinMode(i, INPUT); // External 10k pull-ups.
  for (uint8_t i = 10; i <= 13; ++i) {
    io.digitalWrite(i, LOW);
    io.pinMode(i, OUTPUT);
  }
  uint16_t bits = io.inputGPIO();
  for (uint8_t i = 0; i < 10; ++i) rawInput[i] = stableInput[i] = !(bits & ((uint16_t)1 << i));
  timerDisplay.begin(0x70);
  timerDisplay.setBrightness(3);
  lcd.begin(16, 2);
  lcd.setBacklight(HIGH);
  audioSerial.begin(9600);
  audioReady = audio.begin(audioSerial, true, true);
  if (audioReady) {
    audio.begin(audioSerial, false, false); // Reconfigure without ACK or another reset.
    audio.volume(15);
  }
  else Serial.println(F("DFPlayer unavailable: continuing silently."));
  resetGame();
}

void loop() {
  // Poll every 5ms. DFPlayer sends take about 20ms; the clock accounts for this.
  uint32_t now = millis();
  updateClock(now);
  if (audioReady && audio.available()) audio.read(); // Drain player status messages.
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
    return; // Start/setup button edges never count as puzzle answers.
  }
  if (state != PLAYING) return;
  if (!solved[0]) {
    for (uint8_t i = 0; i < 4 && state == PLAYING; ++i) {
      if (!pressed[i]) continue;
      if (i == symbolAnswer[symbolPosition]) {
        if (++symbolPosition == symbolLength) { completeModule(0); break; }
        sound(2);
      } else {
        symbolPosition = 0;
        penalty();
      }
    }
  }
  if (state != PLAYING) return;
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
  checkSerial(now);
  if (state != PLAYING) return;
  bool lowTime = remainingMs <= 30000;
  io.digitalWrite(WARNING_PIN, lowTime && ((now / 250) % 2));
  if (lowTime && now - lastWarning >= 5000) {
    lastWarning = now;
    sound(7);
  }
}
