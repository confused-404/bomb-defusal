#include <Wire.h>
#include <Adafruit_LiquidCrystal.h>

Adafruit_LiquidCrystal lcd(0);  // Adafruit LCD backpack #292, address 0x20.

void setup() {
  lcd.begin(16, 2);
  lcd.setBacklight(HIGH);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("LCD backpack OK"));
  lcd.setCursor(0, 1);
  lcd.print(F("Backlight ON"));

  delay(1500);
  lcd.setBacklight(LOW);
  delay(500);
  lcd.setBacklight(HIGH);
}

void loop() {
  static uint8_t count = 0;
  lcd.setCursor(15, 1);
  lcd.print(count);
  count = (count + 1) % 10;
  delay(1000);
}
