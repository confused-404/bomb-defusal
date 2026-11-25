const uint8_t POT_PINS[] = {A0, A1, A2};
const uint8_t POT_COUNT = sizeof(POT_PINS) / sizeof(POT_PINS[0]);

void setup() {
  Serial.begin(115200);
  Serial.println(F("Pot test: turn each dial from end to end; digit should span 0-9."));
}

void loop() {
  static uint32_t lastPrint = 0;
  if (millis() - lastPrint < 500) return;
  lastPrint = millis();

  for (uint8_t i = 0; i < POT_COUNT; ++i) {
    const uint16_t raw = analogRead(POT_PINS[i]);
    const uint8_t digit = (uint32_t)raw * 10 / 1024;
    Serial.print(F("A"));
    Serial.print(i);
    Serial.print(F(": "));
    Serial.print(raw);
    Serial.print(F(" -> "));
    Serial.print(digit);
    if (i + 1 < POT_COUNT) Serial.print(F("   "));
  }
  Serial.println();
}
