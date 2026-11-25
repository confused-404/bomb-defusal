# Prototype wiring and quick start

This builds the game in `PLAN.md` from the Adafruit parts you received, the Uno R3 kit,
and pots, toggle switches, and buttons from the lab. The puzzle rules are temporary but playable.
Wire everything with USB unplugged and the battery switched off.

## 1. What each part does

| Job | Part |
| --- | --- |
| Brain | Uno R3 (kit) |
| Inputs, green LEDs, RGB | Adafruit **AW9523** expander + QT-to-male cable |
| Symbol module display | **MAX7219 8×8 LED matrix** |
| Symbol buttons | 4 Kailh clicky key switches with red, yellow, green, blue caps |
| Flash module light | Adafruit RGB backlight (common anode) |
| Flash answer, Start, Reset | 3 push buttons (kit or lab) |
| Serial module screen | Kit LCD1602 + Adafruit LCD backpack |
| Serial module controls | 3 × 10kΩ pots + 3 toggle switches (lab) |
| Countdown | Kit 4-digit 7-segment display + 74HC595 |
| Status / warning LEDs | 3 green + 1 red LEDs |
| Sound | DFPlayer Mini + speaker + microSD card |
| Power | Kit 9V battery → kit power supply module (battery), or Uno USB (bench) |
| Master on/off | Large toggle between the power module's 5V pin and the + rail (section 3) |

**Resistors:** 10 × 10kΩ, 7 × 330Ω (220Ω also works), 9 × 1kΩ.

## 2. Makerspace prep (soldering)

1. **LCD backpack → LCD1602:** solder the backpack onto the LCD's 16-pin header,
   matching pin 1 to pin 1. Leave the backpack's SPI jumper open.
2. **Key switches:** solder a wire to each switch's two metal pins (no polarity), and fit colored caps.
3. **Matrix, AW9523, DFPlayer:** solder header pins if they are not already attached.
   Solder the speaker wires to the DFPlayer's SPK1 and SPK2.
4. **Pots and toggles:** solder wires to them if they don't fit the breadboard.

## 3. Power and breadboard rails

All parts get power from the breadboard rails: **+ rail = 5V** and **− rail = GND**. Every part's GND goes to the − rail.
If your breadboard's rails are split in the middle, bridge them.
Rough estimate: about 250mA on average, with peaks near 400mA when sounds play.

### Battery play: 9V battery → kit power supply module

```text
9V battery ──barrel plug──▶ power module ──5V pin── master switch ──▶ + rail ──▶ Uno 5V pin
                                          └─GND pin──────────────────▶ − rail ──▶ Uno GND
```

1. Set **both** voltage jumpers on the module to **5V**, never 3.3V.
2. Plug the 9V battery's barrel plug into the module's round jack.
3. **Without the faceplate switch:** plug the module onto the end of the breadboard so its pins
   go into the rails, with + on the red rail. The module's own push button is then the master switch.
4. **With the large master toggle:** don't plug the module into the breadboard. Run a jumper from the
   module's **5V** header pin to the toggle, and from the toggle to the **+ rail**.
   Run a second jumper from the module's **GND** pin straight to the **− rail**.
   No battery wires get cut. Leave the module's own button on.
5. Run a jumper from the + rail to the **Uno 5V pin** and one from the − rail to **Uno GND**.
   The Uno runs from the rails this way. Do not use the Uno's barrel jack.

Check with a multimeter before connecting the Uno: the + rail should read about **5V**.

**USB and battery must never be on together.** When you plug in USB to upload, turn the module or
master switch **off** first, and unplug USB before switching it back on.
Otherwise two 5V supplies push against each other through the Uno, which can stress the
module, the Uno, or the computer's USB port.

**Battery life:** a 9V alkaline gives roughly **30–60 minutes** at this draw, and the module's
regulator gets warm. Keep `AUDIO_VOLUME` low. For longer play, use a 6 × AA holder with a
barrel plug (it fits the same jack) or a rechargeable 9V.

### Bench: USB only

With the module off or unplugged, plug the Uno's USB into a computer. The Uno 5V pin then powers the rails.

### If it resets

If the Uno resets or the displays flicker when a sound plays, lower `AUDIO_VOLUME` or replace the battery.
If that isn't enough, add a 100–470µF capacitor across the DFPlayer's VCC and GND,
with the striped leg to GND.
## 4. Uno pins

| Uno pin | Goes to |
| --- | --- |
| 5V, GND | Breadboard + and − rails (powered by the module on battery) |
| A4 (SDA) | AW9523 SDA (QT **blue**), LCD backpack **DAT** |
| A5 (SCL) | AW9523 SCL (QT **yellow**), LCD backpack **CLK** |
| D13 | Matrix **CLK** |
| D11 | Matrix **DIN** |
| D10 | Matrix **CS** (sometimes labeled LOAD) |
| D3, D5, D6, D7 | 7-seg digit 1, 2, 3, 4 (display pins 12, 9, 8, 6) |
| D2 | 74HC595 pin 14 (DS, data) |
| D4 | 74HC595 pin 11 (SH_CP, clock) |
| D8 | 74HC595 pin 12 (ST_CP, latch) |
| A0, A1, A2 | Middle pin of pot 1, 2, 3 |
| A3 | 330Ω → red LED long leg; short leg → GND |
| D1 (TX) | 1kΩ → DFPlayer **RX** |
| D0, D9, D12 | Leave empty. DFPlayer TX stays unconnected. |

**Power for the parts:** connect the + rail to matrix **VCC**, AW9523 VIN (QT **red**),
LCD backpack **5V**, and DFPlayer **VCC**. Connect the − rail to all their GND pins (QT **black**).

**MAX7219 matrix:** use the 5-pin end marked **IN** (VCC, GND, DIN, CS, CLK). Leave the OUT end empty.
During setup, the matrix lights its top-left LED as an orientation check. If a different corner lights,
adjust `MATRIX_ROTATION` or `MATRIX_MIRROR` in the sketch.

**Pots:** connect each pot's two outer pins to the + and − rails. If turning clockwise lowers
the number, swap those two wires. Mark the faceplates 0–9. LCD row 2 shows the live reading,
so you can calibrate the marks against it.

**LCD backpack:** use its terminal block. LAT stays empty. Turn the contrast screw until text appears.

**Audio uses the Uno's USB serial line.** Uploading still works with it connected.
Keep the Serial Monitor closed while playing, because anything sent there also reaches the DFPlayer.

## 5. AW9523 pins

The AW9523 has no built-in pull-ups, so every button and toggle uses this circuit:

```text
+5V ── 10kΩ ──┬── AW9523 pin
              └── button / toggle ── GND
```

For 4-leg buttons, use two **diagonal** legs. For a toggle, wire its middle pin to the AW9523 pin
and one outer pin to GND, leaving the other outer pin empty. The position that connects to GND is **ON**.

| AW pin | Connection |
| --- | --- |
| 0, 1, 2, 3 | Red, yellow, green, blue key switch |
| 4 | Flash answer button |
| 5 | Start button |
| 6 | Reset button (not the Uno's RESET pin) |
| 7, 8, 9 | Toggle 1, 2, 3 |
| 10, 11, 12 | 330Ω → green LED long leg; short leg → GND. Symbol, Flash, Serial modules. |
| 13, 14, 15 | 330Ω → RGB red, green, blue leg |
| RGB longest leg | + rail (common anode) |
| INT, RST | Leave empty |

To identify the RGB legs, connect the longest leg to 5V. Then touch each other leg
to GND through 330Ω and note its color.

## 6. Countdown display (4-digit 7-segment + 74HC595)

The display has 12 pins. Hold it with the digits facing you and the decimal points at the bottom.
Pin 1 is bottom-left, pins 1–6 run left to right along the bottom, and pins 7–12 run right to left along the top.

**74HC595 chip** (notch facing left, pin 1 is bottom-left):

| 595 pin | Connect to |
| --- | --- |
| 16 (VCC), 10 (MR) | + rail |
| 8 (GND), 13 (OE) | − rail |
| 14, 11, 12 | Uno D2, D4, D8 |
| 15 (Q0) | 1kΩ → display pin 11 (segment A) |
| 1 (Q1) | 1kΩ → display pin 7 (B) |
| 2 (Q2) | 1kΩ → display pin 4 (C) |
| 3 (Q3) | 1kΩ → display pin 2 (D) |
| 4 (Q4) | 1kΩ → display pin 1 (E) |
| 5 (Q5) | 1kΩ → display pin 10 (F) |
| 6 (Q6) | 1kΩ → display pin 5 (G) |
| 7 (Q7) | 1kΩ → display pin 3 (decimal point) |
| 9 | Leave empty |

The digit pins (12, 9, 8, 6) go straight to Uno D3, D5, D6, D7.
The 1kΩ resistors keep each Uno pin under about 25mA, so no transistors are needed.
Time shows as `MM.SS`, with the decimal point standing in for the colon.
A part number ending in **…AS** (e.g. 5461AS) is common cathode, which is the default.
One ending in **…BS** is common anode, so set `COMMON_ANODE_DISPLAY = true`.

## 7. Audio files

Use a FAT32 microSD card (32GB or smaller) with a folder `/mp3/` containing your own short clips:

| File | Sound |
| --- | --- |
| `0001.mp3` | Round starts |
| `0002.mp3` | Correct symbol press |
| `0003.mp3` | Wrong answer |
| `0004.mp3` | Module solved |
| `0005.mp3` | Victory |
| `0006.mp3` | Simulated explosion |
| `0007.mp3` | Low-time warning |

## 8. Upload

In the Arduino IDE Library Manager, install **Adafruit AW9523**, **LedControl**,
**Adafruit LiquidCrystal**, and **DFRobotDFPlayerMini**. Accept their dependencies when asked.
Open `arduino_sketch/arduino_sketch.ino`, pick **Arduino Uno** and its port, then upload.

## 9. How to play / prototype expert manual

**Setup** (shown on the LCD): red = Easy, yellow = Medium, green = Hard.
Blue cycles the timer through 3, 5, and 8 minutes.
Start begins the round. Reset returns to setup at any time and keeps your settings.
A wrong symbol press or wrong flash answer costs **10 seconds**. The LCD shows the count as `X:n`.
When all three green LEDs are lit, you win.
Under 30 seconds, the red LED blinks and a warning plays every 5 seconds.

### Symbol Sequence (LED matrix + colored keys)

| Symbol | Looks like | Key |
| --- | --- | --- |
| Box | hollow square | Red |
| Plus | + | Yellow |
| Cross | × | Green |
| Dot | single dot | Blue |

- **Easy:** one enlarged symbol. Press its key.
- **Medium:** symbols appear left and right; press the **right** symbol's key, then the **left** one.
- **Hard:** symbols appear in the four corners. Go top-left, top-right, bottom-left, bottom-right. For each symbol, press its key,
  then its opposite (**red ↔ green, yellow ↔ blue**). That makes 8 presses.
- A mistake restarts this module from the first press.

### RGB Flash Sequence (RGB light + answer button)

A dot lasts 250ms and a dash lasts 750ms. A 2-second dark pause marks where the pattern repeats.
Easy is 1 red flash. Medium is 3 flashes that cycle colors. Hard is 5 flashes of any color.
Values are **red = 1, green = 2, blue = 3**, doubled for a dash. Add them up.
Tap the answer button that many times, then wait 1.5 seconds to submit.
For example, a red dot plus a blue dash is 1 + 6 = 7 taps.

### Serial Code (LCD + pots + toggles)

LCD row 1 shows the code. Row 2 shows the live settings, `D:4 0 7 S:0 1 0`, meaning
dials (pots) 1–3 and switches (toggles) 1–3, where 1 = ON.
Name the code's characters **A B C D E F** from left to right. On Hard, hex A–F mean 10–15.

| Level | Dials 1, 2, 3 | Switches |
| --- | --- | --- |
| Easy (3 digits) | A, B, C | Ignored |
| Medium (6 digits) | Last digit of A+D, B+E, C+F | Switch 1 ON if A+B+C is odd; 2 and 3 ignored |
| Hard (6 hex digits) | Last digit of A XOR D, B XOR E, C XOR F | Switches 1–3 ON if D, E, F are odd; flip all three if A ≥ 8 |

Examples: Easy `407` → dials **4 0 7**. Medium `123456` → **5 7 9**, switch 1 OFF.
Hard `8A31F2` → **9 5 1**, switches **0 0 1**.
Hold the correct settings for 0.75 seconds to solve. Wrong settings never cost time.
A new round always picks a code that doesn't match where the controls are already set.

## 10. First bench test

1. Power up. The LCD should show "Level: Easy", the countdown `05.00`, and the matrix's top-left LED.
   If the matrix stays dark, check VCC, GND, DIN (D11), CLK (D13), and CS (D10), and make sure you used the IN end.
   If it says "AW9523 missing", check the QT cable's wiring to A4/A5, 5V, and GND.
2. Press each color key and check the level on the LCD. Press blue to cycle `03.00`, `05.00`, `08.00`.
3. Start Easy. Turn each pot and flip each toggle, and watch LCD row 2 follow them.
4. Solve each module and check that its green LED lights. Victory shows a check mark on the matrix and "DEFUSED!" on the LCD.
5. Reset. Make deliberate mistakes; each should take 10 seconds. Let the time run out to check the
   warning, `00.00`, and a large X on the matrix with "BOOM! Time's up" on the LCD. Then repeat on Medium and Hard.

## Software checks

Uno build: **17,026 bytes flash (52%)**, **833 bytes global RAM (40%)**, no warnings from the sketch.
The host tests drive the real `loop()` with fake hardware. They cover:
- every symbol layout and Hard flash pattern
- the manual examples
- pot reading
- 7-segment multiplexing
- toggles
- all nine difficulty and timer combinations
- timing edge cases

They do not replace the bench test.

```sh
arduino-cli compile --fqbn arduino:avr:uno arduino_sketch
g++ -std=c++11 -Wall -Wextra -Werror -fsanitize=address,undefined \
  -Itests/fakes tests/prototype_test.cpp -o /tmp/bomb-defusal-test && /tmp/bomb-defusal-test
```
