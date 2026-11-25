const uint8_t DIGIT_PINS[] = {3, 5, 6, 7};
const uint8_t SEG_DATA = 2;
const uint8_t SEG_CLOCK = 4;
const uint8_t SEG_LATCH = 8;
const bool COMMON_ANODE_DISPLAY = false;  // Set true for a part number ending in "BS".

const uint8_t DIGIT_SEGMENTS[] = {
  0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F
};
const uint8_t DECIMAL_POINT = 0x80;
volatile uint8_t displaySegments[4] = {};
volatile uint8_t activeDigit = 0;

ISR(TIMER2_COMPA_vect) {
  digitalWrite(DIGIT_PINS[activeDigit], COMMON_ANODE_DISPLAY ? LOW : HIGH);
  activeDigit = (activeDigit + 1) % 4;

  const uint8_t segments = displaySegments[activeDigit];
  digitalWrite(SEG_LATCH, LOW);
  shiftOut(SEG_DATA, SEG_CLOCK, MSBFIRST,
           COMMON_ANODE_DISPLAY ? (uint8_t)~segments : segments);
  digitalWrite(SEG_LATCH, HIGH);
  digitalWrite(DIGIT_PINS[activeDigit], COMMON_ANODE_DISPLAY ? HIGH : LOW);
}

void showDigits(uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint8_t decimalPoint) {
  uint8_t next[4] = {
    DIGIT_SEGMENTS[a], DIGIT_SEGMENTS[b], DIGIT_SEGMENTS[c], DIGIT_SEGMENTS[d]
  };
  if (decimalPoint < 4) next[decimalPoint] |= DECIMAL_POINT;

  noInterrupts();
  for (uint8_t i = 0; i < 4; ++i) displaySegments[i] = next[i];
  interrupts();
}

void startRefresh() {
  noInterrupts();
  TCCR2A = _BV(WGM21);
  TCCR2B = _BV(CS22) | _BV(CS20);  // 16 MHz / 128.
  OCR2A = 249;                     // 500 Hz digit refresh.
  TIMSK2 = _BV(OCIE2A);
  interrupts();
}

void setup() {
  for (uint8_t i = 0; i < 4; ++i) {
    digitalWrite(DIGIT_PINS[i], COMMON_ANODE_DISPLAY ? LOW : HIGH);
    pinMode(DIGIT_PINS[i], OUTPUT);
  }
  pinMode(SEG_DATA, OUTPUT);
  pinMode(SEG_CLOCK, OUTPUT);
  pinMode(SEG_LATCH, OUTPUT);

  showDigits(1, 2, 3, 4, 1);
  startRefresh();
}

void loop() {
  static uint8_t pattern = 0;
  static uint32_t changedAt = 0;
  if (millis() - changedAt < 1500) return;
  changedAt = millis();

  switch (pattern) {
    case 0: showDigits(1, 2, 3, 4, 1); break;  // 12.34
    case 1: showDigits(8, 8, 8, 8, 4); break;  // All segments on, dot off.
    case 2: showDigits(0, 0, 0, 0, 4); break;
    default: showDigits(1, 1, 1, 1, 4); break;
  }
  pattern = (pattern + 1) % 4;
}
