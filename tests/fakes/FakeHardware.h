#pragma once
#include <stdint.h>
#include <stdlib.h>

// Only for host logic tests. These do not simulate electrical behavior.
const uint8_t A0 = 14, A1 = 15, A2 = 16, A3 = 17;
const uint8_t LOW = 0, HIGH = 1, INPUT = 0, OUTPUT = 1;
const uint16_t ST77XX_BLACK = 0, ST77XX_WHITE = 1, ST77XX_GREEN = 2,
               ST77XX_RED = 3, ST77XX_BLUE = 4;
#define F(s) s
static uint32_t fakeNow = 0;
static uint16_t fakeInputs = 0xffff, fakeOutputs = 0;
static uint16_t fakeAdc[18] = {};
static uint8_t fakeDigital[18] = {};
static bool fakeAudioPresent = false;
static uint8_t fakeTrack = 0;
inline uint32_t millis() { return fakeNow; }
inline uint32_t micros() { return fakeNow * 1000; }
inline void delay(uint32_t ms) { fakeNow += ms; }
inline void randomSeed(uint32_t seed) { srand(seed); }
inline long random(long bound) { return rand() % bound; }
inline int analogRead(uint8_t pin) { return fakeAdc[pin]; }
inline void pinMode(uint8_t, uint8_t) {}
inline void digitalWrite(uint8_t pin, uint8_t value) { fakeDigital[pin] = value; }
struct Printer {
  void begin(uint32_t) {}
  template <typename T> void print(T) {}
  template <typename T> void println(T) {}
};
static Printer Serial;
struct WireStub { void begin() {} };
static WireStub Wire;
struct Adafruit_AW9523 {
  bool begin(uint8_t) { return true; }
  void pinMode(uint8_t, uint8_t) {}
  uint16_t inputGPIO() { return fakeInputs; }
  void digitalWrite(uint8_t pin, bool value) {
    if (value) fakeOutputs |= (uint16_t)1 << pin;
    else fakeOutputs &= ~((uint16_t)1 << pin);
  }
};
struct Adafruit_ST7789 : Printer {
  Adafruit_ST7789(uint8_t, uint8_t, uint8_t) {}
  void init(int, int) {}
  void setRotation(int) {}
  void fillScreen(int) {}
  void setCursor(int, int) {}
  void setTextSize(int) {}
  void setTextColor(int) {}
  void drawCircle(int, int, int, int) {}
  void drawTriangle(int, int, int, int, int, int, int) {}
  void drawRect(int, int, int, int, int) {}
  void drawLine(int, int, int, int, int) {}
};
struct Adafruit_7segment {
  uint8_t digits[5] = {};
  void begin(uint8_t) {}
  void setBrightness(int) {}
  void writeDigitNum(uint8_t position, uint8_t number) { digits[position] = number; }
  void drawColon(bool) {}
  void writeDisplay() {}
};
struct Adafruit_LiquidCrystal : Printer {
  Adafruit_LiquidCrystal(int) {}
  void begin(int, int) {}
  void setBacklight(int) {}
  void clear() {}
  void setCursor(int, int) {}
};
struct SoftwareSerial : Printer { SoftwareSerial(int, int) {} };
struct DFRobotDFPlayerMini {
  bool begin(SoftwareSerial &, bool, bool) { return fakeAudioPresent; }
  void disableACK() {}
  void volume(int) {}
  void stop() { fakeTrack = 0; delay(20); }
  void playMp3Folder(uint8_t track) { fakeTrack = track; delay(20); }
  bool available() { return false; }
  void read() {}
};
