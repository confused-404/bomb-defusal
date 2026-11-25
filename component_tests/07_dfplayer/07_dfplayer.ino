#include <DFRobotDFPlayerMini.h>

DFRobotDFPlayerMini player;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(9600);  // D1 is wired to DFPlayer RX; do not open Serial Monitor.
  delay(1000);

  if (!player.begin(Serial, false, true)) {
    while (true) {
      digitalWrite(LED_BUILTIN, HIGH);
      delay(150);
      digitalWrite(LED_BUILTIN, LOW);
      delay(150);
    }
  }

  player.volume(10);
  delay(300);
  player.playMp3Folder(1);  // Plays /mp3/0001.mp3 once.
}

void loop() {
  digitalWrite(LED_BUILTIN, (millis() / 500) % 2);
}
