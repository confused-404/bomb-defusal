# Prototype wiring and module-by-module guide

This walks through wiring the parts you have, one module at a time. The puzzle rules are
temporary but playable. Keep USB unplugged and the battery switched off while wiring.

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
| Power | Kit 9V battery → kit power supply module (battery), or Uno USB (bench) |
| Master on/off | Large toggle between the power module's 5V pin and the + rail (section 3) |

**Resistors:** 10 × 10kΩ, 7 × 330Ω (220Ω also works), 8 × 1kΩ.

## 2. Makerspace prep (soldering)

1. **LCD backpack → LCD1602:** solder the backpack onto the LCD's 16-pin header,
   matching pin 1 to pin 1. Leave the backpack's SPI jumper open.
2. **Key switches:** solder a wire to each switch's two metal pins (no polarity), and fit colored caps.
3. **Matrix, AW9523:** solder header pins if they are not already attached.
4. **Pots and toggles:** solder wires to them if they don't fit the breadboard.

## 3. Power and breadboard rails

All parts get power from the breadboard rails: **+ rail = 5V** and **− rail = GND**. Every part's GND goes to the − rail.
If your breadboard's rails are split in the middle, bridge them. Current draw and battery
runtime depend on display brightness and the final wiring; verify the supply under load.

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

The power module's regulator can get warm; check it during a full session. Measure battery
runtime with the completed build. For longer play,
use a 6 × AA holder with a barrel plug (it fits the same jack) or a rechargeable 9V.

### Bench: USB only

With the module off or unplugged, plug the Uno's USB into a computer. The Uno 5V pin then powers the rails.

### If it resets

If the Uno resets or the displays flicker, check the battery and the power/GND jumpers.
Confirm with a multimeter that the rail stays near 5V while the displays are running.

## 4. Connect the shared controller

Before adding the puzzle modules, connect the Uno's 5V and GND pins to the breadboard rails.
Connect the AW9523 VIN (QT **red**) to +5V, GND (QT **black**) to GND,
SDA (QT **blue**) to Uno **A4**, and SCL (QT **yellow**) to **A5**.
Leave the AW9523's INT and RST pins empty. That's all the wiring for this step.
(The LCD will later connect to the same A4 and A5 wires. That works because the two
boards have different built-in addresses, so you don't need to set anything.)

### How every button and switch is wired (read now, build later)

You don't wire anything else in this step. The module sections below tell you which
button goes to which AW9523 pin, and each one uses the same three-part hookup:

1. Run a jumper from the AW9523 pin (for example, pin 0) to an empty column on the breadboard.
2. Put a **10kΩ resistor** from that column to the **+ rail**.
3. Put one leg of the button or switch in that same column, and wire its other leg to the **− rail**.

```text
+ rail ── 10kΩ ──┐
                 ├── column ── jumper ── AW9523 pin
− rail ── button ┘
```

Why the resistor is needed: when the button is up, the resistor gently holds the pin at
5V, so the AW9523 reads HIGH. Pressing the button connects the pin straight to GND, so it
reads LOW. Without the resistor, an unpressed pin "floats" and gives random readings.
The AW9523 has no resistor like this built in, so each of AW pins 0–9 needs its own.
You'll use all ten 10kΩ resistors this way.

**2-pin keys and switches (colored keys, toggles):** no polarity, so either pin can go to
the column and the other to the − rail. For a toggle, the position that closes the
switch is **ON**. Mark it on the faceplate.

**4-leg push buttons:** you only wire two of the four legs — one from each side.
Module 2 (section 6) walks through finding the right legs with a multimeter; the same
two-leg rule applies to Start and Reset in section 8.

Now wire and test one module at a time.

## 5. Module 1: Symbol Sequence

### Wire the matrix

Use the MAX7219 module's 5-pin **IN** end; leave OUT empty. Connect VCC to +5V,
GND to GND, **DIN to D11**, **CLK to D13**, and **CS/LOAD to D10**.

### Wire the four keys and status light

Connect the red, yellow, green, and blue key switches to AW9523 pins **0, 1, 2, and 3**,
respectively. One switch terminal goes to its AW pin; the other goes to GND.
The matching 10kΩ pull-up goes from that AW pin to +5V.

Connect the module's green status LED from AW pin **10**, through **330Ω**, to its
long leg. Connect its short leg to GND.

### Check it

Run `03_max7219_matrix` from `component_tests/`. The pixel sweep should visit the full
matrix. The game sketch uses the top-left corner as its orientation reference; if the
orientation is wrong, adjust `MATRIX_ROTATION` or `MATRIX_MIRROR`.
Run `01_aw9523_inputs` to press each colored key and check that only its input changes.
Run `02_aw9523_leds` to check the green status LED.

## 6. Module 2: RGB Flash Sequence

### Wire the RGB light

This is a **common-anode** RGB LED. Connect its longest lead to +5V. Identify the R, G,
and B leads from the part markings or datasheet. Connect each color lead through its
own **330Ω resistor** to AW9523 pins **13 (red), 14 (green), and 15 (blue)**.
These outputs turn on at LOW.

### Wire the answer button (4 legs — you only use 2)

The button's 4 legs stick out on two opposite sides, two per side. Inside the button,
the two legs on the same side are joined together. Pressing the button joins the two
sides. So you wire one leg from each side and leave the other two legs empty.

```text
 A1 ●      ● B1     A1 and A2 are joined inside; B1 and B2 are joined inside.
     [BTN]          Pressing joins the A side to the B side.
 A2 ●      ● B2     Wire one A leg and one B leg — which one of each doesn't matter.
```

**Step 1 — find the joined pairs.** Set the multimeter to continuity (beep mode).
Touch the probes to the two legs on one side of the button. It should beep with the
button just sitting there. Those two are the joined pair on that side.

**Step 2 — pick one leg from each side.** Put one probe on a leg from one side and the
other probe on a leg from the other side. Now it should beep **only while you hold the
button down**. If it beeps without pressing, you picked two legs from the same side —
move one probe to the other side and try again.

**Step 3 — wire it.** Put one of your two chosen legs in an empty breadboard column.
Run a jumper from that column to AW pin **4**, and a **10kΩ resistor** from the same
column to the **+ rail**. Wire the other chosen leg to the **− rail**.

**Step 4 — check it.** Run `01_aw9523_inputs`: pin 4 should read PRESSED only while
the button is held. If it reads PRESSED all the time, you wired two same-side legs;
move the GND wire to a leg on the other side.

Connect the module's green status LED from AW pin **11**, through **330Ω**, to its
long leg; connect the short leg to GND.

### Check it

Run `02_aw9523_leds` to see red, green, and blue one at a time. For a closer check,
run `02b_rgb_led`: the Serial Monitor names each color as it shows. If a color
doesn't match its name, swap those legs on AW pins 13–15. Then run
`01_aw9523_inputs` and check the answer button changes AW pin 4.

## 7. Module 3: Serial Code

### Wire the LCD, dials, and switches

Connect the Adafruit #292 LCD backpack's **5V** and **GND** to the rails, **DAT** to
Uno A4/SDA, and **CLK** to A5/SCL. This shares the bus with the AW9523.
Leave LAT unconnected. Turn the contrast screw until the text is readable.

For each of the three 10kΩ pots, connect the two outside pins to +5V and GND, and the
middle pin (wiper) to **A0, A1, or A2**. If clockwise rotation reduces the reading,
swap that pot's outside wires. Mark the dial from 0 to 9.

**Dials are optional for now.** If the pots aren't wired, leave `dialsWired` set to
`false` in the game sketch: the Serial module then becomes a flip-order puzzle using
just the toggles (see the manual below). Once the pots are in, set `dialsWired = true`
and the module works with dials and switches as the table describes.

The toggles are numbered left to right when facing the controls: **1 (left) → AW9**,
**2 (middle) → AW8**, **3 (right) → AW7**. For each toggle,
connect one pin to its AW pin and the other to GND (either way round). Its 10kΩ
pull-up connects the AW pin to +5V.

Connect the module's green status LED from AW pin **12**, through **330Ω**, to its
long leg; connect the short leg to GND.

### Check it

Run `04_lcd_backpack` and confirm both rows and the backlight work. Run `05_pots` with
the Serial Monitor at 115200 baud; each dial should read from 0 through 9.
Run `01_aw9523_inputs` and check all three toggle states.

## 8. Module 4: Countdown and game controls

### Wire the four-digit display

Hold the display with digits facing you and decimal points at the bottom. Pin 1 is
bottom-left; pins 1–6 run left-to-right along the bottom, and pins 7–12 run right-to-left
along the top.

Place the 74HC595 with its notch facing left and pin 1 at bottom-left. Connect it as follows:

| 74HC595 pin | Connect to |
| --- | --- |
| 16 (VCC), 10 (MR) | +5V rail |
| 8 (GND), 13 (OE) | GND rail |
| 14 (DS), 11 (SH_CP), 12 (ST_CP) | Uno D2, D4, D8 |
| 15 (Q0) | 1kΩ → display pin 11 (segment A) |
| 1 (Q1) | 1kΩ → display pin 7 (B) |
| 2 (Q2) | 1kΩ → display pin 4 (C) |
| 3 (Q3) | 1kΩ → display pin 2 (D) |
| 4 (Q4) | 1kΩ → display pin 1 (E) |
| 5 (Q5) | 1kΩ → display pin 10 (F) |
| 6 (Q6) | 1kΩ → display pin 5 (G) |
| 7 (Q7) | 1kΩ → display pin 3 (decimal point) |
| 9 | Leave unconnected |

Connect the display's digit pins **12, 9, 8, and 6** directly to Uno **D3, D5, D6,
and D7**, respectively. Keep all eight 1kΩ segment resistors. A display part number
ending in `AS` is common-cathode; one ending in `BS` is common-anode. For common-anode,
set `COMMON_ANODE_DISPLAY = true` in both the game sketch and its component test.
The timer displays `MM.SS`.

### Wire Start, Reset, and the warning light

Connect the Start button to AW pin **5** and GND, and Reset to AW pin **6** and GND.
Each pin needs its own 10kΩ pull-up to +5V. Reset is an input button, not the Uno's RESET pin.
These are the same kind of 4-leg buttons as the answer button: use one leg from each
side, exactly as described in section 6, and check them with `01_aw9523_inputs`.

Connect Uno **A3** through **330Ω** to the long leg of the red warning LED; connect
its short leg to GND.

Uno **D0, D1, D9, and D12** remain unused. D0/D1 are available for USB serial;
there is no external device connected to them.

### Check it

Run `06_countdown_display`; it cycles through `12.34`, `8888`, `0000`, and `1111`.
The `8888` pattern checks all seven segments on all digits. Then run
`06b_countdown_timer` to watch it count down from `05.00`, like a real round.
Run `01_aw9523_inputs` for Start and Reset, and `02_aw9523_leds` for the red warning LED.

## 9. Upload and test

Before uploading over USB, switch off and disconnect the battery supply. Install
**Adafruit AW9523**, **LedControl**, and **Adafruit LiquidCrystal**
in Arduino IDE's Library Manager; accept their dependencies.

Test components individually using [`component_tests/README.md`](../component_tests/README.md).
Upload the sketch in each test's matching folder. For tests that use the Serial
Monitor, open it at the baud rate shown in the test guide.

When the individual checks pass, open `arduino_sketch/arduino_sketch.ino`, select
**Arduino Uno** and its port, and upload the full game.

## 10. How to play / prototype expert manual

**Setup:** the LCD shows the selected difficulty and duration together, e.g. `Easy / 5 min`.
Press **red** to cycle Easy → Medium → Hard. Press **blue** to cycle 3 → 5 → 8 minutes.
The seven-segment display also previews the time. Start begins the round. Reset returns to setup at any time and keeps your settings.
A wrong symbol press, flash answer, or toggle flip costs **10 seconds**.
When all three green LEDs are lit, you win.
Under 30 seconds, the red LED blinks. Feedback and end states use LEDs and displays only.

### Symbol Sequence (LED matrix + colored keys)

| Symbol | Looks like | Key |
| --- | --- | --- |
| Box | hollow square | Red |
| Plus | + | Yellow |
| Cross | × | Green |
| Dot | single dot | Blue |

- **Easy:** one enlarged symbol. Press its key.
- **Medium:** four corner symbols. Press the four matching keys in **any order**. Repeated symbols need repeated presses.
- **Hard:** the same four-corner layout. Press the matching keys **in order**: top-left, top-right, bottom-left, bottom-right.
- A mistake restarts this module from the first press.

### RGB Flash Sequence (RGB light + answer button)

A dot lasts 250ms and a dash lasts 750ms. A 2-second dark pause marks where the pattern repeats.
Easy has one red flash. Medium has one each of red, green, and blue. Hard adds
yellow, cyan, and magenta, for six flashes total. The colors are shuffled on Medium and Hard.

| Color | Short | Long |
| --- | --- | --- |
| Red | 1 | 2 |
| Green | 2 | 4 |
| Blue | 3 | 6 |
| Yellow | 4 | 8 |
| Cyan | 5 | 10 |
| Magenta | 6 | 12 |

Add the flash values from one pattern. Tap the answer button that many times, then
wait 1.5 seconds to submit. Short red + long blue = 7 taps.

### Serial Code (LCD + pots + toggles)

**While the dials are unwired, this module is a flip-order puzzle instead:** the LCD
shows a short hex code. The LCD's `S:` row shows live switch positions, left to right.
On Medium and Hard, `D:247` means the three simulated dial readings are 2, 4, 7.
Use those numbers as clues; there are no physical dials to set.

| Level | Read the hex code | Add before decoding |
| --- | --- | --- |
| Easy (2 digits) | Left to right | Nothing |
| Medium (3 digits) | Left to right | The first dial reading to every digit |
| Hard (4 digits) | Right to left | Dial 1, dial 2, dial 3, then dial 1 again |

Convert A–F to 10–15. For each digit (after the addition), divide by 3 and use
the remainder: **0 = left (AW9), 1 = middle (AW8), 2 = right (AW7)**.
For Easy, you can also use this lookup:

| Hex digit | Switch to flip |
| --- | --- |
| `0 3 6 9 C F` | Left |
| `1 4 7 A D` | Middle |
| `2 5 8 B E` | Right |

- Easy `0A`: left, middle.
- Medium `A5C`, `D:247`: add 2 to 10, 5, 12 → 12, 7, 14 → left, middle, right.
- Hard `A5C7`, `D:247`: read 7, 12, 5, 10; add 2, 4, 7, 2 → 9, 16, 12, 12 → left, middle, left, left.

Either direction counts. For a repeated switch, flip it once each way. A wrong
flip or flips registered together cost 10 seconds and restart the sequence;
leave the switches where they are.

The rest of this section only applies when the pots are wired and `dialsWired` is true.

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

## 11. Full game bench test

After the individual tests pass, reconnect any modules you temporarily disconnected.
Turn off the battery module, unplug USB, then power up from the battery. The LCD should
show "Easy / 5 min", the countdown `05.00`, and the matrix's top-left LED.

- If the matrix stays dark, check VCC, GND, DIN (D11), CLK (D13), and CS (D10), and make sure you used the IN end.
- If the LCD says "AW9523 missing", check the QT cable's wiring to A4/A5, 5V, and GND.
- All three green status LEDs should be off in Setup and immediately after Start.
  If one stays on, run `02_aw9523_leds` and check the Serial Monitor at 115200 baud.
  The green LEDs should light only during their named steps: Symbol (AW10),
  Flash (AW11), Serial (AW12). With power off, check the stuck LED's wiring:
  AW pin → 330Ω → LED long leg; LED short leg → GND.

1. Press red to cycle the difficulty. Press blue to cycle `03.00`, `05.00`, `08.00`.
2. Start Easy. The LCD should show a hex code like `Code:A5`. Decode it (see the
   Serial Code manual) and flip the toggles in that order; the Serial module's green
   LED lights. A flip out of order costs 10 seconds. (Once the dials are wired and
   `dialsWired` is true, LCD row 2 should follow the pots and toggles instead.)
3. Solve each module and check that its green LED lights. Victory shows a check mark on the matrix and "DEFUSED!" on the LCD.
4. Reset. Make deliberate mistakes; each should take 10 seconds. Let the time run out to check the
   warning, `00.00`, and a large X on the matrix with "BOOM! Time's up" on the LCD. Then repeat on Medium and Hard.

## Software checks

Uno build: **15,114 bytes flash (46%)**, **619 bytes global RAM (30%)**, no warnings from the sketch.
All six component-test sketches compile for Uno.
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
