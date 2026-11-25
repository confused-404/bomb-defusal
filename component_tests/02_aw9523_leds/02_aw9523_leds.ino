#include <Wire.h>
#include <Adafruit_AW9523.h>

const uint8_t WARNING_LED = A3;
const uint8_t FIRST_AW_LED = 10;
const uint8_t LED_COUNT = 7;  // A3 warning, AW10-12 green, AW13-15 RGB.
const char *const LED_NAMES[LED_COUNT] = {
  "Red warning (A3)", "Symbol green (AW10)", "Flash green (AW11)",
  "Serial green (AW12)", "RGB red (AW13)", "RGB green (AW14)", "RGB blue (AW15)"
};

Adafruit_AW9523 io;

void allOff() {
  digitalWrite(WARNING_LED, LOW);
  for (uint8_t pin = 10; pin <= 12; ++pin) io.digitalWrite(pin, LOW);
  for (uint8_t pin = 13; pin <= 15; ++pin) io.digitalWrite(pin, HIGH);
}

void showLed(uint8_t index) {
  allOff();
  if (index == 0) {
    digitalWrite(WARNING_LED, HIGH);
  } else {
    const uint8_t pin = FIRST_AW_LED + index - 1;
    io.digitalWrite(pin, pin >= 13 ? LOW : HIGH);  // RGB is active-low.
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(WARNING_LED, OUTPUT);
  digitalWrite(WARNING_LED, LOW);
  Wire.begin();

  if (!io.begin(0x58)) {
    Serial.println(F("AW9523 not found at I2C 0x58. Check power and A4/A5."));
    while (true) delay(100);
  }

  for (uint8_t pin = 10; pin <= 12; ++pin) {
    io.digitalWrite(pin, LOW);
    io.pinMode(pin, OUTPUT);
  }
  for (uint8_t pin = 13; pin <= 15; ++pin) {
    io.digitalWrite(pin, HIGH);
    io.pinMode(pin, OUTPUT);
  }
  allOff();
  Serial.println(F("LED test: each LED lights for one second, in order."));
}

void loop() {
  static uint8_t index = 0;
  static uint32_t changedAt = 0;
  if (millis() - changedAt < 1000) return;
  changedAt = millis();
  showLed(index);
  Serial.println(LED_NAMES[index]);
  index = (index + 1) % LED_COUNT;
}
