#include <Wire.h>
#include <Adafruit_AW9523.h>

// RGB backlight test (Adafruit #6158, common anode).
// Longest leg to +5V; red/green/blue legs through 330Ω to AW pins 13/14/15.
// A pin set LOW lights that color.
const uint8_t RGB_FIRST = 13;  // 13 red, 14 green, 15 blue.

Adafruit_AW9523 io;

// One bit per channel: bit 0 red, bit 1 green, bit 2 blue.
const uint8_t COLOR_COUNT = 8;
const uint8_t COLORS[COLOR_COUNT] = {1, 2, 4, 3, 5, 6, 7, 0};
const char *const COLOR_NAMES[COLOR_COUNT] = {
  "RED", "GREEN", "BLUE", "YELLOW (red+green)",
  "MAGENTA (red+blue)", "CYAN (green+blue)", "WHITE (all)", "OFF"
};

void showColor(uint8_t channels) {
  for (uint8_t i = 0; i < 3; ++i) {
    io.digitalWrite(RGB_FIRST + i, (channels & (1 << i)) ? LOW : HIGH);
  }
}

void setup() {
  Serial.begin(115200);
  Wire.begin();
  if (!io.begin(0x58)) {
    Serial.println(F("AW9523 not found at I2C 0x58. Check power and A4/A5."));
    while (true) delay(100);
  }
  for (uint8_t i = 0; i < 3; ++i) {
    io.digitalWrite(RGB_FIRST + i, HIGH);  // Off.
    io.pinMode(RGB_FIRST + i, OUTPUT);
  }
  Serial.println(F("RGB test. Watch that the LED matches the named color."));
}

void loop() {
  static uint8_t index = 0;
  static uint32_t changedAt = 0;
  if (millis() - changedAt < 5000) return;
  changedAt = millis();
  showColor(COLORS[index]);
  Serial.println(COLOR_NAMES[index]);
  index = (index + 1) % COLOR_COUNT;
}
