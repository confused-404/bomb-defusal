#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../arduino_sketch/arduino_sketch.ino"

const uint16_t RGB_BITS = 0xE000, STATUS_BITS = 0x1C00;
const uint8_t FACEPLATE_SWITCH_PINS[] = {9, 8, 7};  // Left, middle, right on this build.

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
uint16_t potFor(uint8_t digit) { return (digit * 1024 + 512) / 10; }  // Middle of a digit's range.
void setControls(const uint8_t *d, const uint8_t *s) {
  for (uint8_t i = 0; i < 3; ++i) {
    fakeAdc[POT_PINS[i]] = potFor(d[i]);
    setInput(FACEPLATE_SWITCH_PINS[i], s[i]);
  }
}
void prepare(uint8_t level = 0, uint8_t minutes = 1) {
  dialsWired = true;
  fakeInputs = 0xffff;
  advance(50);
  difficulty = level; timerChoice = minutes;
  resetGame();
  press(START);
  assert(state == PLAYING);
}
void setSerial() { setControls(dialAnswer, switchAnswer); advance(50); }
void ruleTests() {
  uint8_t s[] = {0, 1, 2, 3}, answer[8];
  assert(makeSymbolAnswer(0, s, answer) == 1 && answer[0] == 0);
  for (int layout = 0; layout < 256; ++layout) {
    for (int i = 0; i < 4; ++i) s[i] = (layout >> (2 * i)) & 3;
    for (uint8_t level = 1; level <= 2; ++level) {
      assert(makeSymbolAnswer(level, s, answer) == 4);
      assert(memcmp(s, answer, 4) == 0);
    }
  }
  const uint8_t clue[] = {10, 5, 12, 7}, simulated[] = {2, 4, 7};
  assert(decodeToggle(0, clue, simulated, 2, 0) == 1);
  assert(decodeToggle(1, clue, simulated, 3, 0) == 0);
  const uint8_t hardOrder[] = {0, 1, 0, 0};
  for (uint8_t i = 0; i < 4; ++i) assert(decodeToggle(2, clue, simulated, 4, i) == hardOrder[i]);
  const uint8_t palette[] = {1, 2, 4, 3, 6, 5};
  for (uint8_t i = 0; i < 6; ++i) {
    rgbOn(i);
    assert(((~fakeOutputs & RGB_BITS) >> RGB_FIRST) == palette[i]);
  }
  uint8_t mixed[] = {3, 4, 5}, mixedLengths[] = {0, 1, 0};
  assert(makeFlashAnswer(3, mixed, mixedLengths) == 20);
  uint8_t colors[] = {0, 2}, lengths[] = {0, 1};
  assert(makeFlashAnswer(2, colors, lengths) == 7);  // Manual example.
  for (int pattern = 0; pattern < 7776; ++pattern) {  // All five-flash primary-color patterns.
    uint8_t c[5], d[5]; int n = pattern, expected = 0;
    const uint8_t values[] = {1, 2, 2, 4, 3, 6};
    for (int i = 0; i < 5; ++i) {
      int token = n % 6; n /= 6;
      c[i] = token / 2; d[i] = token % 2; expected += values[token];
    }
    assert(makeFlashAnswer(5, c, d) == expected);
  }
  uint8_t d[3], sw[3];
  uint8_t easy[] = {4, 0, 7, 0, 0, 0};
  makeSerialAnswer(0, easy, d, sw);
  assert(d[0] == 4 && d[1] == 0 && d[2] == 7);
  uint8_t medium[] = {1, 2, 3, 4, 5, 6};
  makeSerialAnswer(1, medium, d, sw);
  assert(d[0] == 5 && d[1] == 7 && d[2] == 9 && sw[0] == 0);
  uint8_t hex[] = {8, 10, 3, 1, 15, 2};
  makeSerialAnswer(2, hex, d, sw);
  assert(d[0] == 9 && d[1] == 5 && d[2] == 1);
  assert(sw[0] == 0 && sw[1] == 0 && sw[2] == 1);
  // Pot: endpoints, every digit's middle from every previous digit, boundary dead zone.
  for (uint8_t prev = 0; prev < 10; ++prev) {
    assert(readPotDigit(0, prev) == 0);
    assert(readPotDigit(1023, prev) == 9);
    for (uint8_t digit = 0; digit < 10; ++digit) assert(readPotDigit(potFor(digit), prev) == digit);
  }
  assert(readPotDigit(102, 0) == 0 && readPotDigit(104, 0) == 0 && readPotDigit(120, 0) == 1);
  assert(readPotDigit(110, 1) == 1 && readPotDigit(101, 1) == 1 && readPotDigit(90, 1) == 0);
}
void displayTests() {
  for (uint8_t i = 0; i < 4; ++i) fakeDigital[DIGIT_PINS[i]] = HIGH;  // As setup() leaves them.
  lastShownSeconds = 65535;
  showTime(300);  // "05.00"
  const uint8_t expected[] = {0x3F, 0x6D | 0x80, 0x3F, 0x3F};
  for (uint8_t n = 0; n < 8; ++n) {
    TIMER2_COMPA_vect();
    uint8_t lit = 255, count = 0;
    for (uint8_t i = 0; i < 4; ++i) if (fakeDigital[DIGIT_PINS[i]] == LOW) { lit = i; ++count; }
    assert(count == 1);  // Exactly one digit on at a time.
    assert(fakeShifted == expected[lit] && fakeDigital[SEG_LATCH] == HIGH);
  }
  showTime(65);  // "01.05"
  assert(displaySegments[0] == 0x3F && displaySegments[1] == (0x06 | 0x80));
  assert(displaySegments[2] == 0x3F && displaySegments[3] == 0x6D);
}
void matrixTests() {
  // Rows as sent to the module (bit 7 = column 0), so this checks the real output.
  auto sent = [](uint8_t y) {
    uint8_t v = 0;
    for (uint8_t x = 0; x < 8; ++x) if (matrix.rows[y] & (0x80 >> x)) v |= 1 << x;
    return v;
  };
  difficulty = 0; symbols[0] = 0;  // Easy: one big Box.
  drawSymbolPuzzle();
  const uint8_t easy[8] = {0, 0x7E, 0x7E, 0x66, 0x66, 0x7E, 0x7E, 0};
  for (uint8_t y = 0; y < 8; ++y) assert(sent(y) == easy[y]);
  difficulty = 2;  // Hard: Box TL, Plus TR, Cross BL, Dot BR.
  symbols[0] = 0; symbols[1] = 1; symbols[2] = 2; symbols[3] = 3;
  drawSymbolPuzzle();
  const uint8_t hard[8] = {0x47, 0xE5, 0x47, 0, 0, 0x05, 0x42, 0x05};
  for (uint8_t y = 0; y < 8; ++y) assert(sent(y) == hard[y]);
  difficulty = 1;
  drawSymbolPuzzle();
  for (uint8_t y = 0; y < 8; ++y) assert(sent(y) == hard[y]);
  // All four symbols are different from each other.
  for (uint8_t i = 0; i < 4; ++i) for (uint8_t j = i + 1; j < 4; ++j)
    assert(memcmp(SYMBOL_ROWS[i], SYMBOL_ROWS[j], 3) != 0);
}
void gameTests() {
  fakeAdc[A0] = 1023;
  setup();
  assert(!dialsWired && dials[0] == 0);  // Ignore the unwired pin even at boot.
  assert(state == SETUP && lastShownSeconds == 300);
  assert(TIMSK2 == _BV(OCIE2A) && OCR2A == 249);  // 500 Hz display refresh.
  const uint8_t firstWave[] = {0, 0x60, 0x10, 0x80, 0x08, 0x01, 0x06, 0};
  const uint8_t nextWave[] = {0, 0x60, 0x80, 0x10, 0x01, 0x08, 0x06, 0};
  assert(matrix.awake && memcmp(matrix.rows, firstWave, 8) == 0);
  assert(io.modes[RGB_FIRST] == AW9523_LED_MODE);
  assert(io.levels[RGB_FIRST] == 48 && io.levels[RGB_FIRST + 1] == 0);
  advance(85);
  assert(io.levels[RGB_FIRST] == 45 && io.levels[RGB_FIRST + 1] == 3);
  assert(memcmp(matrix.rows, firstWave, 8) == 0);
  advance(5);
  assert(memcmp(matrix.rows, nextWave, 8) == 0);
  advance(15 * 90);
  assert(memcmp(matrix.rows, firstWave, 8) == 0);
  assert((fakeOutputs & RGB_BITS) == RGB_BITS && (fakeOutputs & STATUS_BITS) == 0);
  assert(lcd.rows[0] == "Easy / 5 min    ");
  assert(lcd.rows[1] == "Red:Lvl Blue:Min");
  press(0); assert(difficulty == 1 && lcd.rows[0] == "Medium / 5 min  ");
  press(0); assert(difficulty == 2 && lcd.rows[0] == "Hard / 5 min    ");
  press(0); assert(difficulty == 0);
  press(3); assert(lastShownSeconds == 480 && lcd.rows[0] == "Easy / 8 min    ");
  press(3); assert(lastShownSeconds == 180);
  press(3); assert(lastShownSeconds == 300);
  // Bounce is ignored; holding Start creates no repeated edge.
  setInput(START, true); advance(10); setInput(START, false); advance(50);
  assert(state == SETUP);
  setInput(START, true); advance(100);
  assert(state == PLAYING);
  for (uint8_t i = 0; i < 3; ++i) assert(io.modes[RGB_FIRST + i] == OUTPUT && io.levels[RGB_FIRST + i] == 0);
  uint8_t puzzleFrame[8];
  memcpy(puzzleFrame, matrix.rows, 8);
  advance(500);
  assert(memcmp(matrix.rows, puzzleFrame, 8) == 0);
  press(RESET); advance(100);
  assert(state == SETUP && memcmp(matrix.rows, nextWave, 8) == 0);
  setInput(START, false); advance(50);

  for (uint8_t level = 0; level < 3; ++level) {
    for (uint8_t timer = 0; timer < 3; ++timer) {
      prepare(level, timer);
      assert(lastShownSeconds == ROUND_SECONDS[timer]);
      assert(!serialSettingsMatch());  // Never starts solved.
      uint8_t wrong = (symbolAnswer[0] + 1) % 4;
      if (level == 1) {
        uint8_t colors = 0;
        for (uint8_t key : symbolAnswer) colors |= 1 << key;
        if (colors == 15) { wrong = symbolAnswer[0]; press(wrong); }
        else { for (wrong = 0; colors & (1 << wrong); ++wrong) {} }
      }
      uint32_t before = remainingMs;
      press(wrong);
      assert(before - remainingMs == 10100 && symbolPosition == 0);
      for (uint8_t i = 0; i < symbolLength; ++i) press(symbolAnswer[i]);
      assert(solved[0] && (fakeOutputs & (1 << STATUS_FIRST)));
      before = remainingMs;
      press(wrong); assert(before - remainingMs == 100);  // Solved module ignores input.
      for (uint8_t i = 0; i < flashAnswer; ++i) press(LARGE);
      advance(1600);
      assert(solved[1] && (fakeOutputs & (1 << (STATUS_FIRST + 1))));
      assert((fakeOutputs & RGB_BITS) == RGB_BITS);
      setSerial(); advance(900);
      assert(state == WON && solved[2] && (fakeOutputs & (1 << (STATUS_FIRST + 2))));
      assert(matrix.rows[4] == 0x88 && matrix.rows[6] == 0x20);  // Check mark.
      assert(fakeDigital[WARNING_PIN] == LOW);
      press(RESET);
      assert(state == SETUP && !solved[0] && !solved[1] && !solved[2]);
      assert(!tapCount && !symbolPosition && !serialMatching);
      assert((fakeOutputs & STATUS_BITS) == 0);
    }
  }
  prepare();
  flashAnswer = 2;
  uint32_t beforeFlash = remainingMs;
  press(LARGE); advance(1600);
  assert(beforeFlash - remainingMs == 11700 && tapCount == 0 && !solved[1]);
  beforeFlash = remainingMs;
  setInput(LARGE, true); advance(2000);  // Held: must not submit.
  assert(beforeFlash - remainingMs == 2000 && tapCount == 1);
  press(RESET);
  assert(state == SETUP && tapCount == 0);
  setInput(LARGE, false); advance(50);

  // A round never starts already solved, whatever the controls are set to.
  for (int trial = 0; trial < 200; ++trial) {
    uint8_t d[3] = {(uint8_t)(trial % 10), (uint8_t)(trial / 10 % 10), 3};
    uint8_t s[3] = {(uint8_t)(trial & 1), (uint8_t)(trial >> 1 & 1), (uint8_t)(trial >> 2 & 1)};
    setControls(d, s);
    prepare(trial % 3);
    assert(!serialSettingsMatch());
    advance(900); assert(!solved[2]);
  }
  // Toggles and pots are read live; wrong settings cost nothing; correct ones must hold 750ms.
  prepare(2);
  uint32_t beforeDials = remainingMs;
  setSerial(); advance(300);
  assert(serialMatching && !solved[2] && beforeDials - remainingMs == 350);
  setInput(FACEPLATE_SWITCH_PINS[0], !switchAnswer[0]); advance(100);
  assert(!serialMatching && switches[0] == !switchAnswer[0]);
  setSerial(); advance(400); assert(!solved[2]);
  advance(500); assert(solved[2]);
  // Easy ignores the toggles.
  prepare(0);
  uint8_t flipped[3] = {1, 0, 1};
  setControls(dialAnswer, flipped); advance(900);
  assert(solved[2]);

  // Flash timing through the real loop.
  prepare(1);
  flashLength = 2; flashIndex = 0; flashOn = true; flashChanged = fakeNow;
  flashColors[0] = 0; flashColors[1] = 2;
  flashLong[0] = 0; flashLong[1] = 1;
  rgbOff(); rgbOn(0);
  auto lit = [](uint8_t color) { return !(fakeOutputs & (1 << (RGB_FIRST + color))); };
  advance(245); assert(flashOn && lit(0));
  advance(5); assert(!flashOn && !lit(0));
  advance(245); assert(!flashOn);
  advance(5); assert(flashOn && flashIndex == 1 && lit(2));
  advance(745); assert(flashOn);
  advance(5); assert(!flashOn && !lit(2));
  advance(1995); assert(!flashOn);
  advance(5); assert(flashOn && flashIndex == 0);

  prepare();
  remainingMs = 29900;
  advance(5);
  { bool seenOn = false, seenOff = false;  // Warning LED blinks.
    for (int i = 0; i < 100; ++i) { advance(5); (fakeDigital[WARNING_PIN] ? seenOn : seenOff) = true; }
    assert(seenOn && seenOff); }
  remainingMs = 5000;
  press((symbolAnswer[0] + 1) % 4);
  assert(state == LOST && remainingMs == 0 && fakeDigital[WARNING_PIN] == HIGH);
  for (uint8_t y = 0; y < 8; ++y) assert(matrix.rows[y] == ((0x80 >> y) | (1 << y)));  // Big X.
  assert(matrix.awake && lcd.backlight && lcd.rows[0] == "BOOM! Time's up ");
  advance(250);
  assert(!matrix.awake && !lcd.backlight);
  press(RESET);
  assert(state == SETUP && fakeDigital[WARNING_PIN] == LOW && matrix.awake && lcd.backlight);
  prepare();
  remainingMs = 5;
  advance(5);
  advance(1500);
  assert(state == LOST && matrix.awake && lcd.backlight);
  advance(750);
  assert(matrix.awake && lcd.backlight);
  prepare();
  solved[0] = solved[1] = true;
  setSerial(); assert(serialMatching);
  remainingMs = 10; advance(1000);
  assert(state == LOST && !solved[2] && lastShownSeconds == 0);
  // Time elapsed before completing a module cannot become victory after expiry.
  prepare();
  solved[0] = solved[1] = true;
  remainingMs = 10; fakeNow += 20;
  completeModule(2); assert(state == LOST && !solved[2]);
  // No dials: the manual decodes the hex clue; initial switch positions don't count.
  dialsWired = false;
  fakeInputs = 0xffff;
  advance(50);
  difficulty = 1; timerChoice = 1;
  resetGame();
  setInput(FACEPLATE_SWITCH_PINS[0], true); advance(50);
  press(START);
  uint32_t beforeMs = remainingMs;
  advance(1000);
  assert((fakeOutputs & STATUS_BITS) == 0 && toggleOrderPos == 0 && beforeMs - remainingMs == 1000);
  assert(state == PLAYING && !solved[2] && toggleOrderLen == 3);
  assert(toggleLast[0] && !toggleLast[1] && !toggleLast[2]);
  const auto dialClue = lcd.rows[1].substr(7);
  assert(dialClue == " D:" + std::to_string(virtualDials[0] * 100 + virtualDials[1] * 10 + virtualDials[2]) + "   ");
  for (uint8_t i = 0; i < toggleOrderLen; ++i) {
    assert(toggleCode[i] < 16 && toggleOrder[i] == (toggleCode[i] + virtualDials[0]) % 3);
  }
  // A flip out of order costs 10 seconds and restarts the sequence.
  const uint8_t manualSwitch[] = {1,2,3,1,2,3,1,2,3,1,2,3,1,2,3,1};
  uint8_t wrongToggle = (manualSwitch[(toggleCode[0] + virtualDials[0]) % 3] % 3);
  beforeMs = remainingMs;
  uint8_t wrongPin = FACEPLATE_SWITCH_PINS[wrongToggle];
  setInput(wrongPin, !stableInput[wrongPin]); advance(50);
  assert(toggleOrderPos == 0 && beforeMs - remainingMs == 10050);
  assert(lcd.rows[1].substr(7) == dialClue);
  // Several toggles in the same scan have no observable order.
  beforeMs = remainingMs;
  for (uint8_t pin : FACEPLATE_SWITCH_PINS) setInput(pin, !stableInput[pin]);
  advance(50);
  assert(beforeMs - remainingMs == 10050 && toggleOrderPos == 0 && (fakeOutputs & STATUS_BITS) == 0);
  // Flip the toggles in the shown order (either direction) to solve.
  for (uint8_t i = 0; i < toggleOrderLen; ++i) {
    uint8_t pin = FACEPLATE_SWITCH_PINS[manualSwitch[(toggleCode[i] + virtualDials[0]) % 3] - 1];
    setInput(pin, !stableInput[pin]); advance(50);
  }
  assert(solved[2] && (fakeOutputs & (1 << (STATUS_FIRST + 2))));
  for (uint8_t i = 0; i < symbolLength; ++i) press(symbolAnswer[i]);
  for (uint8_t i = 0; i < flashAnswer; ++i) press(LARGE);
  advance(1600);
  assert(state == WON);
  // Same four symbols: reverse order is valid only on Medium, repeats count.
  for (uint8_t level = 1; level <= 2; ++level) {
    prepare(level);
    uint8_t paletteSeen = 0;
    for (uint8_t i = 0; i < flashLength; ++i) paletteSeen |= 1 << flashColors[i];
    assert(flashLength == (level == 1 ? 3 : 6));
    assert(paletteSeen == (level == 1 ? 7 : 63));
    const uint8_t answers[] = {0, 1, 0, 3};
    memcpy(symbolAnswer, answers, 4);
    uint32_t before = remainingMs;
    press(3); press(0); press(1); press(0);
    assert(solved[0] == (level == 1));
    assert(level == 1 ? before - remainingMs == 400 : before - remainingMs == 10400);
    if (level == 1) {
      prepare(level);
      memcpy(symbolAnswer, answers, 4);
      press(1); press(1);
      assert(symbolPosition == 0 && symbolUsed == 0 && !solved[0]);
      for (uint8_t key : answers) press(key);
      assert(solved[0]);
    }
  }
  // millis() rollover, using the Uno's unsigned 32-bit arithmetic.
  prepare();
  fakeNow = UINT32_MAX - 100;
  lastTick = lastScan = fakeNow;
  remainingMs = 1000;
  advance(200); assert(state == PLAYING && remainingMs == 800);
  advance(800); assert(state == LOST && remainingMs == 0);
}
int main() {
  ruleTests(); displayTests(); gameTests(); matrixTests();
  puts("PASS: rules, pot reading, 7-seg multiplexing, matrix symbols, 9 difficulty/timer games, toggles, debounce, "
       "penalties, reset, dwell, flash timing, warning LED, expiry, toggle order, rollover");
}
