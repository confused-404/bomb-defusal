#include <Wire.h>
#include <Adafruit_AW9523.h>

const uint8_t INPUT_COUNT = 10;
const char *const INPUT_NAMES[INPUT_COUNT] = {
  "Red key", "Yellow key", "Green key", "Blue key",
  "Flash answer", "Start", "Reset",
  "Toggle 3", "Toggle 2", "Toggle 1"
};

Adafruit_AW9523 io;
bool lastPressed[INPUT_COUNT];

void printState(uint8_t pin, bool pressed) {
  Serial.print(F("AW pin "));
  Serial.print(pin);
  Serial.print(F(" ("));
  Serial.print(INPUT_NAMES[pin]);
  Serial.print(F("): "));
  if (pin <= 6) {
    Serial.println(pressed ? F("PRESSED") : F("released"));
  } else {
    Serial.println(pressed ? F("ON / closed") : F("OFF / open"));
  }
}

void setup() {
  Serial.begin(115200);
  Wire.begin();

  if (!io.begin(0x58)) {
    Serial.println(F("AW9523 not found at I2C 0x58. Check power and A4/A5."));
    while (true) delay(100);
  }

  for (uint8_t pin = 0; pin < INPUT_COUNT; ++pin) {
    io.pinMode(pin, INPUT);
  }

  Serial.println(F("AW9523 input test. Each input needs a 10k pull-up to 5V."));
  const uint16_t bits = io.inputGPIO();
  for (uint8_t pin = 0; pin < INPUT_COUNT; ++pin) {
    lastPressed[pin] = !(bits & ((uint16_t)1 << pin));  // LOW means pressed/ON.
    printState(pin, lastPressed[pin]);
  }
}

void loop() {
  static uint32_t lastRead = 0;
  if (millis() - lastRead < 20) return;
  lastRead = millis();

  const uint16_t bits = io.inputGPIO();
  for (uint8_t pin = 0; pin < INPUT_COUNT; ++pin) {
    const bool pressed = !(bits & ((uint16_t)1 << pin));
    if (pressed != lastPressed[pin]) {
      lastPressed[pin] = pressed;
      printState(pin, pressed);
    }
  }
}
