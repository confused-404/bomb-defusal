// Countdown timer test: counts down 05.00 -> 00.00 on the 4-digit display,
// with the decimal point blinking every half second. At zero it blinks
// 00.00 a few times, then restarts. Same wiring and refresh as the game.
const uint8_t DIGIT_PINS[] = {3, 5, 6, 7};
const uint8_t SEG_DATA = 2;
const uint8_t SEG_CLOCK = 4;
const uint8_t SEG_LATCH = 8;
const bool COMMON_ANODE_DISPLAY = false;  // Set true for a part number ending in "BS".

const uint16_t START_SECONDS = 300;  // 05.00, like the game's default round.

const uint8_t DIGIT_SEGMENTS[] = {
  0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F
};
const uint8_t DECIMAL_POINT = 0x80;
volatile uint8_t displaySegments[4] = {};

ISR(TIMER2_COMPA_vect) {
  static uint8_t digit = 0;
  digitalWrite(DIGIT_PINS[digit], COMMON_ANODE_DISPLAY ? LOW : HIGH);
  digit = (digit + 1) % 4;

  const uint8_t segments = displaySegments[digit];
  digitalWrite(SEG_LATCH, LOW);
  shiftOut(SEG_DATA, SEG_CLOCK, MSBFIRST,
           COMMON_ANODE_DISPLAY ? (uint8_t)~segments : segments);
  digitalWrite(SEG_LATCH, HIGH);
  digitalWrite(DIGIT_PINS[digit], COMMON_ANODE_DISPLAY ? HIGH : LOW);
}

void startRefresh() {
  noInterrupts();
  TCCR2A = _BV(WGM21);
  TCCR2B = _BV(CS22) | _BV(CS20);  // 16 MHz / 128.
  OCR2A = 249;                     // 500 Hz digit refresh.
  TIMSK2 = _BV(OCIE2A);
  interrupts();
}

void showTime(uint16_t seconds, bool decimalPoint) {
  const uint8_t minutes = seconds / 60, secs = seconds % 60;
  displaySegments[0] = DIGIT_SEGMENTS[minutes / 10];
  displaySegments[1] = DIGIT_SEGMENTS[minutes % 10] | (decimalPoint ? DECIMAL_POINT : 0);
  displaySegments[2] = DIGIT_SEGMENTS[secs / 10];
  displaySegments[3] = DIGIT_SEGMENTS[secs % 10];
}

void blank() {
  for (uint8_t i = 0; i < 4; ++i) displaySegments[i] = 0;
}

void setup() {
  for (uint8_t i = 0; i < 4; ++i) {
    digitalWrite(DIGIT_PINS[i], COMMON_ANODE_DISPLAY ? LOW : HIGH);
    pinMode(DIGIT_PINS[i], OUTPUT);
  }
  pinMode(SEG_DATA, OUTPUT);
  pinMode(SEG_CLOCK, OUTPUT);
  pinMode(SEG_LATCH, OUTPUT);
  showTime(START_SECONDS, true);
  startRefresh();
}

void loop() {
  static uint16_t remaining = START_SECONDS;
  static uint32_t lastSecond = 0;
  static uint32_t blinkChanged = 0;
  static uint8_t blinks = 0;
  static bool blanked = false;
  const uint32_t now = millis();

  if (remaining > 0) {
    if (now - lastSecond >= 1000) {
      lastSecond = now;
      --remaining;
    }
    showTime(remaining, (now / 500) % 2);  // Decimal point ticks twice a second.
    return;
  }

  // Zero reached: blink 00.00 six times, then restart.
  if (now - blinkChanged < 300) return;
  blinkChanged = now;
  blanked = !blanked;
  if (blanked) blank();
  else showTime(0, true);
  if (!blanked && ++blinks >= 6) {
    blinks = 0;
    remaining = START_SECONDS;
    lastSecond = now;
  }
}
