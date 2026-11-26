#include <Wire.h>

// I2C bus scanner. Run this first when an I2C board isn't detected.
// Expected on this build: 0x58 (AW9523) and 0x20 (LCD backpack, if wired).
void setup() {
  Serial.begin(115200);
  Wire.begin();
  Serial.println(F("I2C scanner. Expect 0x58 (AW9523), 0x20 (LCD backpack)."));
  Serial.println();
}

void loop() {
  uint8_t found = 0;
  for (uint8_t address = 1; address < 127; ++address) {
    Wire.beginTransmission(address);
    if (Wire.endTransmission() != 0) continue;
    Serial.print(F("Found a device at 0x"));
    if (address < 16) Serial.print('0');
    Serial.println(address, HEX);
    ++found;
  }
  if (!found) {
    Serial.println(F("Nothing found. Check SDA->A4, SCL->A5, VIN->+5V, GND->GND."));
  }
  Serial.println();
  delay(2000);
}
