#include <assert.h>
#include <stdio.h>
#include "../arduino_sketch/arduino_sketch.ino"

void advance(uint32_t ms) {
  for (uint32_t i = 0; i < ms; i += 5) { fakeNow += 5; loop(); }
}
void setInput(uint8_t pin, bool down) {
  if (down) fakeInputs &= ~((uint16_t)1 << pin);
  else fakeInputs |= (uint16_t)1 << pin;
}
void press(uint8_t pin) {
  setInput(pin, true); advance(50);
  setInput(pin, false); advance(50);
}
void prepare(uint8_t level = 0, uint8_t minutes = 1) {
  fakeInputs = 0xffff;
  advance(50);
  difficulty = level; timerChoice = minutes;
  resetGame();
  press(START);
  assert(state == PLAYING);
}
void setSerial() {
  for (uint8_t i = 0; i < 3; ++i) {
    fakeAdc[POT_PINS[i]] = (potAnswer[i] * 1024 + 512) / 10;
    setInput(7 + i, switchAnswer[i]);
  }
}
void ruleTests() {
  uint8_t s[] = {0, 1, 2, 3}, answer[8];
  assert(makeSymbolAnswer(0, s, answer) == 1 && answer[0] == 0);
  assert(makeSymbolAnswer(1, s, answer) == 2 && answer[0] == 1 && answer[1] == 0);
  const uint8_t hard[] = {0, 2, 1, 3, 2, 0, 3, 1};
  assert(makeSymbolAnswer(2, s, answer) == 8);
  for (uint8_t i = 0; i < 8; ++i) assert(answer[i] == hard[i]);
  // Exhaust every symbol arrangement; every answer remains within four buttons.
  for (int layout = 0; layout < 256; ++layout) {
    for (int i = 0; i < 4; ++i) s[i] = (layout >> (2 * i)) & 3;
    for (uint8_t level = 0; level < 3; ++level) {
      uint8_t count = makeSymbolAnswer(level, s, answer);
      assert(count == (level == 0 ? 1 : level == 1 ? 2 : 8));
      for (uint8_t i = 0; i < count; ++i) assert(answer[i] < 4);
      if (level == 1) assert(answer[0] == s[1] && answer[1] == s[0]);
      if (level == 2) for (uint8_t i = 0; i < 4; ++i) {
        assert(answer[i * 2] == s[i]);
        assert((answer[i * 2 + 1] + 2) % 4 == s[i]);
      }
    }
  }
  uint8_t colors[] = {0, 2}, lengths[] = {0, 1};
  assert(makeFlashAnswer(2, colors, lengths) == 7); // Manual example.
  // Exhaust all 6^5 possible Hard flash patterns, including max 30 taps.
  for (int pattern = 0; pattern < 7776; ++pattern) {
    uint8_t c[5], d[5]; int n = pattern, expected = 0;
    const uint8_t values[] = {1, 2, 2, 4, 3, 6};
    for (int i = 0; i < 5; ++i) {
      int token = n % 6; n /= 6;
      c[i] = token / 2; d[i] = token % 2; expected += values[token];
    }
    assert(makeFlashAnswer(5, c, d) == expected);
  }
  uint8_t pots[3], toggles[3];
  uint8_t easy[] = {4, 0, 7, 0, 0, 0};
  makeSerialAnswer(0, easy, pots, toggles);
  assert(pots[0] == 4 && pots[1] == 0 && pots[2] == 7);
  uint8_t medium[] = {1, 2, 3, 4, 5, 6};
  makeSerialAnswer(1, medium, pots, toggles);
  assert(pots[0] == 5 && pots[1] == 7 && pots[2] == 9 && toggles[0] == 0);
  uint8_t hex[] = {8, 10, 3, 1, 15, 2};
  makeSerialAnswer(2, hex, pots, toggles);
  assert(pots[0] == 9 && pots[1] == 5 && pots[2] == 1);
  assert(toggles[0] == 0 && toggles[1] == 0 && toggles[2] == 1);
  // ADC endpoints, every pot center from every previous digit, boundary deadband.
  for (uint8_t prev = 0; prev < 10; ++prev) {
    assert(readPotDigit(0, prev) == 0);
    assert(readPotDigit(1023, prev) == 9);
    for (uint8_t digit = 0; digit < 10; ++digit)
      assert(readPotDigit((digit * 1024 + 512) / 10, prev) == digit);
  }
  assert(readPotDigit(102, 0) == 0 && readPotDigit(104, 0) == 0);
  assert(readPotDigit(110, 1) == 1 && readPotDigit(101, 1) == 1);
}
void gameTests() {
  setup();
  assert(state == SETUP && lastShownSeconds == 300 && !audioReady);
  press(1); assert(difficulty == 1);
  press(2); assert(difficulty == 2);
  press(0); assert(difficulty == 0);
  press(3); assert(lastShownSeconds == 480);
  press(3); assert(lastShownSeconds == 180);
  press(3); assert(lastShownSeconds == 300);
  // Button bounce is ignored; holding Start creates no repeated edge.
  setInput(START, true); advance(10); setInput(START, false); advance(50);
  assert(state == SETUP);
  setInput(START, true); advance(100);
  assert(state == PLAYING);
  press(RESET); advance(100);
  assert(state == SETUP); // Start remains held and must not restart.
  setInput(START, false); advance(50);

  for (uint8_t level = 0; level < 3; ++level) {
    for (uint8_t timer = 0; timer < 3; ++timer) {
      prepare(level, timer);
      assert(lastShownSeconds == ROUND_SECONDS[timer]);
      uint8_t wrong = (symbolAnswer[0] + 1) % 4;
      uint32_t before = remainingMs;
      press(wrong);
      assert(strikes == 1 && before - remainingMs == 10100 && symbolPosition == 0);
      for (uint8_t i = 0; i < symbolLength; ++i) press(symbolAnswer[i]);
      assert(solved[0] && (fakeOutputs & (1 << 10)));
      uint8_t oldStrikes = strikes;
      press(wrong); assert(strikes == oldStrikes); // Solved input is ignored.
      for (uint8_t i = 0; i < flashAnswer; ++i) press(LARGE);
      advance(1600);
      assert(solved[1] && (fakeOutputs & (1 << 11)));
      for (uint8_t i = 0; i < 3; ++i) assert(fakeDigital[RGB_PINS[i]] == HIGH);
      setSerial(); advance(900);
      assert(state == WON && solved[2] && (fakeOutputs & (1 << 12)));
      press(RESET);
      assert(state == SETUP && !solved[0] && !solved[1] && !solved[2]);
      assert(!tapCount && !symbolPosition && !serialMatching && !strikes);
      assert((fakeOutputs & 0x3c00) == 0);
    }
  }
  prepare();
  flashAnswer = 2;
  press(LARGE); advance(1600);
  assert(strikes == 1 && tapCount == 0 && !solved[1]);
  // A held large button must not submit until after release + quiet period.
  setInput(LARGE, true); advance(2000);
  assert(tapCount == 1 && strikes == 1);
  press(RESET);
  assert(state == SETUP && tapCount == 0);
  setInput(LARGE, false); advance(50);

  prepare(2);
  setSerial(); advance(300);
  assert(serialMatching && !solved[2]);
  setInput(7, !switchAnswer[0]); advance(100);
  assert(!serialMatching);
  setSerial(); advance(400); assert(!solved[2]);
  advance(500); assert(solved[2]);

  // Measure a known dot/dash pattern through the real loop entry point.
  prepare(1);
  flashLength = 2; flashIndex = 0; flashOn = true; flashChanged = fakeNow;
  flashColors[0] = 0; flashColors[1] = 2;
  flashLong[0] = 0; flashLong[1] = 1;
  rgbOff(); digitalWrite(RGB_PINS[0], LOW);
  advance(245); assert(flashOn && fakeDigital[RGB_PINS[0]] == LOW);
  advance(5); assert(!flashOn && fakeDigital[RGB_PINS[0]] == HIGH);
  advance(245); assert(!flashOn);
  advance(5); assert(flashOn && flashIndex == 1 && fakeDigital[RGB_PINS[2]] == LOW);
  advance(745); assert(flashOn);
  advance(5); assert(!flashOn);
  advance(1995); assert(!flashOn);
  advance(5); assert(flashOn && flashIndex == 0);

  prepare();
  remainingMs = 29900; lastWarning = fakeNow - 5000;
  fakeAudioPresent = audioReady = true;
  advance(5); assert(fakeTrack == 7);
  remainingMs = 5000;
  press((symbolAnswer[0] + 1) % 4);
  assert(state == LOST && remainingMs == 0 && fakeTrack == 6);
  press(RESET); assert(state == SETUP && fakeTrack == 0);
  prepare();
  solved[0] = solved[1] = true;
  setSerial(); advance(100); assert(serialMatching);
  remainingMs = 10; advance(1000);
  assert(state == LOST && !solved[2] && lastShownSeconds == 0);
  // Expiry while a sound command is transmitting cannot become victory.
  prepare();
  solved[0] = solved[1] = true;
  remainingMs = 10; fakeNow += 20;
  completeModule(2); assert(state == LOST && !solved[2]);
  // millis rollover, using the same unsigned 32-bit arithmetic as the Uno.
  audioReady = false; // Isolate exact elapsed time from simulated audio-send delays.
  prepare();
  fakeNow = UINT32_MAX - 100;
  lastTick = lastScan = fakeNow;
  remainingMs = 1000;
  advance(200); assert(state == PLAYING && remainingMs == 800);
  advance(800); assert(state == LOST && remainingMs == 0);
}
int main() {
  ruleTests(); gameTests();
  puts("PASS: puzzle rules, 9 difficulty/timer games, debounce, penalties, reset, dwell, flash/audio timing, expiry, rollover");
}
