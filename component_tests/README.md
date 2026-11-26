# Component wiring tests

Use these small sketches to check the hardware **one module at a time**, before
running the full game. Each folder contains a standalone Arduino sketch. Upload
the matching `.ino` from that folder; do not combine the sketches.

## Before testing

1. **Check the supply first.** With the Uno and components disconnected, set both
   power-module jumpers to **5V**. Use a multimeter to confirm the module produces
   about 5V at its output. Check that the master switch turns that output on and
   off. Never put 9V directly on the breadboard 5V rail.
2. For bench tests, turn the battery module **off** and use **USB only**. Connect
   Uno 5V and GND to the breadboard rails so the parts under test get power.
   Never have USB and the battery module on at the same time.
3. Use **115200 baud** where a test calls for the Serial Monitor.
4. Follow the full pin and resistor details in the [wiring guide](../planning/WIRING.md). Keep each
   test's power off while changing wires.

## Run the tests

| Test sketch | What to connect | What you should see |
| --- | --- | --- |
| `00_i2c_scanner/00_i2c_scanner.ino` | Just the Uno plus whatever I²C boards are wired (SDA A4, SCL A5, 5V, GND). | Serial Monitor (115200 baud) lists every address found, every 2 seconds. Expect `0x58` (AW9523) and `0x20` (LCD backpack if wired). "Nothing found" means a bus wiring or power problem. |
| `01_aw9523_inputs/01_aw9523_inputs.ino` | AW9523 power and I²C (SDA A4, SCL A5); each AW pin 0–9 gets its own 10kΩ pull-up to 5V and a switch/button to GND. | Serial Monitor lists all ten inputs. Press/close each control and its state changes. |
| `02_aw9523_leds/02_aw9523_leds.ino` | Red warning LED on A3 through 330Ω; green LEDs on AW pins 10–12 through 330Ω; common-anode RGB on AW pins 13–15 through 330Ω. | The warning LED, three green LEDs, then RGB red/green/blue light in sequence. |
| `02b_rgb_led/02b_rgb_led.ino` | RGB common-anode LED: longest leg to 5V; red/green/blue legs through 330Ω to AW pins 13/14/15; AW9523 power and I²C. | Red, green, blue, yellow, magenta, cyan, white, then off, changing every 1.5 seconds. The Serial Monitor (115200 baud) names each color; if a color doesn't match its name, the legs are on the wrong AW pins. |
| `03_max7219_matrix/03_max7219_matrix.ino` | Matrix VCC/GND; DIN D11, CLK D13, CS/LOAD D10. Use the module's **IN** connector. | A corner pixel appears, the rows sweep, then every pixel is visited. |
| `03b_matrix_fill_ring/03b_matrix_fill_ring.ino` | Same wiring as `03_max7219_matrix`. | Every LED lights for 2 seconds, then only the outer ring for 2 seconds, repeating. Any LED that stays dark when all should be on, or any lit LED inside the ring, points to a wiring or seating problem. |
| `03c_matrix_coordinates/03c_matrix_coordinates.ino` | Same wiring as `03_max7219_matrix`. Hold the matrix the way it will be mounted. | The first row fills one LED at a time, holds, and repeats. Note which corner it starts in and which way it runs; the game treats that starting LED as top-left. |
| `03d_matrix_game_screens/03d_matrix_game_screens.ino` | Same wiring as `03_max7219_matrix`. | Every matrix screen the game uses, in order: top-left setup LED; big Box, Plus, Cross, Dot (Easy); all four in the corners (Medium and Hard); check mark (win); big X (loss). Then it repeats. |
| `04_lcd_backpack/04_lcd_backpack.ino` | LCD backpack power and I²C (DAT/SDA A4, CLK/SCL A5). | Both text rows appear; the backlight briefly turns off and back on. |
| `05_pots/05_pots.ino` | Pot wipers to A0, A1, A2; each pot's outer pins to 5V and GND. | Serial Monitor shows each raw reading and a digit from 0–9 as you turn the pots. |
| `06_countdown_display/06_countdown_display.ino` | 4-digit display and 74HC595 wired as described under Module 4 in the wiring guide. | The display cycles through `12.34`, `8888`, `0000`, and `1111`; the dot should appear on `12.34`. |
| `06b_countdown_timer/06b_countdown_timer.ino` | Same wiring as `06_countdown_display`. | Counts down from `05.00` to `00.00`, one second at a time, with the dot blinking twice a second. At zero it blinks `00.00` six times and restarts. |

### Notes for individual tests

- In the AW9523 input test, AW pins 0–3 are the colored keys, 4 is the flash
  answer button, 5 is Start, 6 is Reset, and 7–9 are the right, middle, and left toggles. A button press or
  toggle ON closes the input to GND, so the reading is **LOW**.
- The AW9523 does not provide the input pull-ups; install all ten 10kΩ resistors.
  Do not connect an input directly to 5V when its button or toggle is closed to GND.
- AW pins 10–12 drive the green LEDs active-high. The RGB LED is common-anode:
  its longest lead goes to 5V, and AW pins 13–15 sink current active-low.
- If the seven-segment display is common-anode (part number ending in `BS`),
  change `COMMON_ANODE_DISPLAY` to `true` near the top of its sketch.
- `05_pots` only applies once the three dials are wired. Until then the Serial module
  is a toggle flip-order puzzle; see `dialsWired` in `arduino_sketch.ino`.

## System test

`08_system_test/08_system_test.ino` checks everything at once, using the full-game wiring.
Each input drives an output you can see. Open the Serial Monitor at 115200 baud to see each change
listed as well.

| Do this | You should see |
| --- | --- |
| Nothing | Matrix: top-left LED only. All other LEDs off. |
| Hold a colored key | Matrix: that key's symbol (red Box, yellow Plus, green Cross, blue Dot). RGB: red, yellow (red + green), green, or blue. |
| Hold the flash button | RGB lights white (all three colors). |
| Hold Start | Red warning LED on. |
| Hold Reset | All three green LEDs on. |
| Flip toggle 1, 2, or 3 ON | Green LED 1, 2, or 3 on. |
| Turn pot 1, 2, or 3 | Countdown digit 1, 2, or 3 follows it (0–9). |
| Press any AW input | Countdown digit 4 shows that AW pin number (0–9). |

The LCD shows `In:` followed by one character per AW pin 0–9 (`1` = pressed or ON,
`.` = released or OFF) and, on row 2, the dial and switch readings (switches in left-to-right order). Parts you haven't
wired yet simply won't respond. If the AW9523 isn't found, the LCD says so and only
the pots and countdown work.

After the separate checks pass, assemble the modules and run the full game sketch
from `arduino_sketch/arduino_sketch.ino`.
