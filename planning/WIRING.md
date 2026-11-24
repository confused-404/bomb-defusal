# Prototype wiring and quick start

This implements `PLAN.md` with **temporary, playable puzzle rules** below.
Use an **Arduino Uno R3 (5V)**. Wire with power disconnected.

## 1. Parts to add / check

- Choose the **Adafruit AW9523 breakout** (not PCF8574). It handles all buttons,
  puzzle toggles, and status LEDs. It has **no internal pull-ups**.
- Add **10 × 10kΩ** input pull-ups, **7 × 330Ω** LED resistors,
  **1 × 1kΩ** DFPlayer RX resistor, three **10kΩ linear pots**.
- [Display #865](https://www.adafruit.com/product/865) is a **bare display**.
  Add its compatible [HT16K33 backpack #877](https://www.adafruit.com/product/877)
  and solder it using the backpack guide, or use assembled
  [display #5599](https://www.adafruit.com/product/5599).
  Do **not** connect bare #865 directly to the Uno.
- [TFT #4311](https://www.adafruit.com/product/4311) is ST7789, 320×240,
  with 5V-safe power and logic.
- [RGB #6158](https://www.adafruit.com/product/6158) is a **common-anode RGB
  backlight**, not a NeoPixel. The longest lead is +; identify R/G/B from its
  datasheet or test each through 330Ω.
- [Speaker #6486](https://www.adafruit.com/product/6486) is 8Ω / 2W.
  DFPlayer SPK outputs drive it directly, no extra amplifier needed at moderate volume.

## 2. Power first

Use one regulated **5V supply rated at least 2A**, split into a 5V rail and GND rail:

```text
5V supply + ── master switch ── +5V rail ── Uno 5V + peripherals
5V supply - ─────────────────── GND rail ── Uno GND + all grounds
```

Do not put 5V into VIN or the barrel jack. With this external 5V-pin supply,
**disconnect ordinary USB before applying power**. For uploading, disconnect the
external supply and power from USB, preferably with DFPlayer unplugged.
To debug on external power, use a proper USB VBUS-isolating adapter.
Keep the supply wiring short and outside the breadboard's small signal jumpers.

Add **470µF, ≥10V** across DFPlayer VCC/GND (striped negative lead to GND), plus
**100nF ceramic** nearby. Add 100nF across the expander power rails.
Check breadboard rail breaks and polarity with a meter before powering.
This is only a sound-and-light game, with no ignition or energetic components.

## 3. Uno connections

All peripherals share GND. Pin names below are labels, not physical pin numbers.

| Uno | Connect to |
| --- | --- |
| 5V rail | TFT VIN / 3–5V, LCD backpack +5V/VIN, HT16K33 VCC/+, AW9523 VIN, DFPlayer VCC |
| A4 / SDA | AW9523 SDA, LCD backpack SDA/DAT, HT16K33 SDA/D |
| A5 / SCL | AW9523 SCL, LCD backpack SCL/CLK, HT16K33 SCL/C |
| D13 | TFT SCK |
| D11 | TFT MOSI |
| D10 | TFT CS |
| D9 | TFT D/C |
| D8 | TFT RST |
| D3 | 330Ω → RGB red cathode |
| D5 | 330Ω → RGB green cathode |
| D6 | 330Ω → RGB blue cathode |
| 5V rail | RGB common anode (longest lead) |
| A0, A1, A2 | Pot 1, 2, 3 middle pins (wipers), respectively |
| D4 (TX) | 1kΩ → DFPlayer RX |
| D2 (RX) | DFPlayer TX |

Each pot's outside pins go to **5V and GND**. Swap them if clockwise decreases
the value. Mark the faceplates **0–9**, increasing clockwise. Leave A3 unconnected
(random seed), D0/D1 free for USB serial, and D7/D12 unused.

Leave TFT BL and 3Vo unconnected (backlight defaults on).
Leave TFT MISO unconnected, tie SDCS to 5V, and do not insert a TFT SD card.
Only the DFPlayer uses a microSD card.
Solder [LCD backpack #292](https://learn.adafruit.com/i2c-spi-lcd-backpack)
to a standard 16-pin 16×2 character LCD; leave its SPI-selection jumper open.
Adjust its contrast screw until text appears.

**Shared I²C bus:** AW9523 `0x58`, Adafruit LCD `0x20`, HT16K33 `0x70`.
Leave address jumpers open. QT cable colors: black=GND, red=5V,
blue=SDA, yellow=SCL. Keep bus wires short.
The sketch uses `Adafruit_LiquidCrystal`, not a generic PCF8574 LCD library.

## 4. AW9523 connections

For **each** input, use this same circuit:

```text
5V ── 10kΩ ──┬── AW input pin
             └── button / toggle ── GND
```

Pressed/closed = LOW. Use normally-open button contacts. For four-legged tactile
buttons, choose contacts that connect only when pressed, not an already-connected pair.
For an SPDT toggle, connect common to the input and one throw to GND;
leave the other throw unconnected. Label closed-to-GND as **ON / 1**.
The game's Reset is an ordinary input, **not** the Uno RESET pin.

| AW pin | Control / indicator |
| --- | --- |
| 0 | Red symbol button |
| 1 | Yellow symbol button |
| 2 | Green symbol button |
| 3 | Blue symbol button |
| 4 | Large RGB-answer button |
| 5 | Start button |
| 6 | Reset button |
| 7, 8, 9 | Serial toggles 1, 2, 3 |
| 10 | 330Ω → symbol green LED anode; cathode → GND |
| 11 | 330Ω → flash green LED anode; cathode → GND |
| 12 | 330Ω → serial green LED anode; cathode → GND |
| 13 | 330Ω → warning red LED anode; cathode → GND |
| 14, 15, INT, RST | Unconnected |

The sketch uses ordinary GPIO mode, so **keep the LED resistors**.
The AW warning pin 13 is not Uno D13, which is the TFT clock.

## 5. Audio and upload

Connect the speaker **between DFPlayer SPK1 and SPK2**. Neither speaker wire
goes to GND. Leave DAC, BUSY, USB, and key pins unused.
Start at the sketch's volume 15/30; reduce it if sound distorts.

On a FAT32 microSD card (≤32GB), create `/mp3/` with:

| File | Sound |
| --- | --- |
| `0001.mp3` | Round starts |
| `0002.mp3` | Correct symbol press |
| `0003.mp3` | Wrong answer |
| `0004.mp3` | Module solved |
| `0005.mp3` | Victory |
| `0006.mp3` | Simulated explosion |
| `0007.mp3` | Low-time warning |

Supply your own short clips; no audio files are included. Missing DFPlayer
does not prevent gameplay. Audio initialization may take a few seconds.

In Arduino IDE Library Manager install:
**Adafruit AW9523**, **Adafruit ST7735 and ST7789 Library**,
**Adafruit LED Backpack Library**, **Adafruit LiquidCrystal**,
**DFRobotDFPlayerMini**, and their offered dependencies (GFX and BusIO).
Wire, SPI, and SoftwareSerial come with the Uno core.
Open `arduino_sketch/arduino_sketch.ino`, select **Arduino Uno**, select the port,
and upload.

## 6. Play / prototype expert manual

In setup: **red=Easy, yellow=Medium, green=Hard**, blue cycles **3/5/8 minutes**.
Default: Easy, 5 minutes. Start locks settings. Reset returns to setup from any
state and keeps the chosen settings. All three green LEDs mean victory.
Wrong symbol/flash answers subtract **10 seconds**; there is no strike limit.
Under 30 seconds, the red warning blinks and audio warns every 5 seconds.

### Symbol Sequence

| Symbol | Color |
| --- | --- |
| Circle | Red |
| Triangle | Yellow |
| Square | Green |
| Plus sign | Blue |

- **Easy:** press the one symbol's color.
- **Medium:** press the **right symbol, then left** (even if identical).
- **Hard:** read top-left, top-right, bottom-left, bottom-right.
  For each symbol press its color, then its opposite:
  **red ↔ green, yellow ↔ blue**. Eight presses total.
- A mistake restarts this module's answer from the beginning.

### RGB Flash Sequence

A dot lights for **250ms**; a dash for **750ms**.
Flashes have 250ms dark gaps; the pattern ends with **2 seconds dark** and repeats.
Easy has 1 red flash, Medium 3 cycling-color flashes, Hard 5 independently colored flashes.

Add the flash values: **red=1, green=2, blue=3**, doubled for a dash.
Tap the large button that many times, keeping gaps under 1.5 seconds.
After the final release, wait **1.5 seconds** to submit.
Example: red dot + blue dash = **1 + 6 = 7 taps**.
A wrong submission clears the tap count; the pattern keeps repeating.

### Serial Code

Label code characters left-to-right **A B C D E F**; pots/toggles are 1–3.
Use decimal digits normally; on Hard, hex A–F mean 10–15.
“Last digit” means remainder after division by 10.

| Level | Pot settings | Toggle settings |
| --- | --- | --- |
| Easy (3 digits) | A, B, C | All ignored |
| Medium (6 digits) | Last digit of A+D, B+E, C+F | Toggle 1 ON if A+B+C is odd; 2–3 ignored |
| Hard (6 hex digits) | Last digit of A XOR D, B XOR E, C XOR F | 1–3 ON if D, E, F respectively are odd; invert all three if A ≥ 8 |

XOR is bitwise exclusive-or (use a hex XOR table for the expert).
Examples: Easy `407` → pots **4,0,7**; Medium `123456` → **5,7,9**, switch 1 OFF;
Hard `8A31F2` → **9,5,1**, switches **OFF,OFF,ON**.
Hold the complete correct configuration for **750ms** to solve.
Intermediate/wrong settings incur no penalty, so turning pots is safe.

## 7. First bench test

1. Check unpowered wiring; power up. Confirm setup on TFT/LCD and `05:00`.
   If TFT reports AW9523 missing, fix its power/I²C wiring first.
2. Test all difficulty buttons and cycle the timer through `03:00`, `05:00`, `08:00`.
3. Start Easy. Solve the symbol, count/answer the red flashes, and set the three
   pots to the displayed digits. Check each green LED, then victory.
4. Reset; deliberately give wrong symbol and flash answers. Each loses 10 seconds.
   Leave the game running to verify warning, `00:00`, and loss.
5. Repeat on Medium/Hard. Test Reset while tapping, while holding a button,
   after partial symbol input, after a solved module, and after each end state.

Hardware behavior, pin polarity, audio playback, and pot faceplate calibration
still require this physical bench test.

## Software checks

The Uno build passes: **28,916 bytes flash (89%)**, **979 bytes global RAM (47%)**.
Heap and stack use additional RAM. Compiler warnings are in the installed
LED Backpack library and AVR core, not this sketch.
Host tests cover all 256 symbol arrangements, all 7,776 Hard flash patterns,
the manual examples, all nine difficulty/timer combinations, and state/timing edge cases.
They use fake peripherals, so they do not replace bench testing.

Run from the project directory:

```sh
arduino-cli compile --fqbn arduino:avr:uno arduino_sketch
g++ -std=c++11 -Wall -Wextra -Werror -fsanitize=address,undefined \
  -Itests/fakes tests/prototype_test.cpp -o /tmp/bomb-defusal-test
/tmp/bomb-defusal-test
```
