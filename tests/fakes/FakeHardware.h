#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <string>
#include <cassert>

// Only for host logic tests. These do not simulate electrical behavior.
const uint8_t A0 = 14, A1 = 15, A2 = 16, A3 = 17;
const uint8_t LOW = 0, HIGH = 1, INPUT = 0, OUTPUT = 1, MSBFIRST = 1;
#define F(s) s
#define ISR(name) void name()
#define _BV(bit) (1 << (bit))
static uint8_t TCCR2A, TCCR2B, OCR2A, TIMSK2;
const uint8_t WGM21 = 1, CS22 = 2, CS20 = 0, OCIE2A = 1;
inline void noInterrupts() {}
inline void interrupts() {}

static uint32_t fakeNow = 0;
static uint16_t fakeInputs = 0xffff, fakeOutputs = 0;
static uint16_t fakeAdc[18] = {};
static uint8_t fakeDigital[18] = {};
static uint8_t fakeShifted = 0;
inline uint32_t millis() { return fakeNow; }
inline uint32_t micros() { return fakeNow * 1000; }
inline void delay(uint32_t ms) { fakeNow += ms; }
inline void randomSeed(uint32_t seed) { srand(seed); }
inline long random(long bound) { return rand() % bound; }
inline int analogRead(uint8_t pin) { return fakeAdc[pin]; }
inline void pinMode(uint8_t, uint8_t) {}
inline void digitalWrite(uint8_t pin, uint8_t value) { fakeDigital[pin] = value; }
inline void shiftOut(uint8_t, uint8_t, uint8_t, uint8_t value) { fakeShifted = value; }
struct Printer {
  void begin(uint32_t) {}
  template <typename T> void print(T) {}
  template <typename T> void println(T) {}
};
struct WireStub { void begin() {} };
static WireStub Wire;
const uint8_t AW9523_LED_MODE = 0x99;
struct Adafruit_AW9523 {
  uint8_t modes[16] = {}, levels[16] = {};
  void analogWrite(uint8_t pin, uint8_t level) { levels[pin] = level; }
  bool begin(uint8_t) { return true; }
  void pinMode(uint8_t pin, uint8_t mode) { modes[pin] = mode; }
  uint16_t inputGPIO() { return fakeInputs; }
  void digitalWrite(uint8_t pin, bool value) {
    if (value) fakeOutputs |= (uint16_t)1 << pin;
    else fakeOutputs &= ~((uint16_t)1 << pin);
  }
};
struct LedControl {
  uint8_t rows[8] = {};
  bool awake = false;
  LedControl(int, int, int, int) {}
  void shutdown(int, bool off) { awake = !off; }
  void setIntensity(int, int) {}
  void clearDisplay(int) { for (uint8_t &r : rows) r = 0; }
  void setRow(int, int row, uint8_t value) { rows[row] = value; }
};
struct Adafruit_LiquidCrystal : Printer {
  Adafruit_LiquidCrystal(int) {}
  void begin(int, int) {}
  bool backlight = false;
  void setBacklight(int on) { backlight = on; }
  std::string rows[2] = {std::string(16, ' '), std::string(16, ' ')};
  int col = 0, row = 0;
  void clear() {
    for (auto &line : rows) line.assign(16, ' ');
    setCursor(0, 0);
  }
  void setCursor(int x, int y) { col = x; row = y; }
  void print(char c) { if (col < 16) rows[row][col++] = c; }
  void print(const char *s) { while (*s) print(*s++); }
  void print(int n) { print(std::to_string(n).c_str()); }
};
