# Bomb Defusal Game

Inspired by *Keep Talking and Nobody Explodes*, this is a two-player tabletop game built around an Arduino Uno and three physical puzzle modules.
The **defuser** can see and operate the modules and describes their clues to the **expert**.
The expert uses a manual to interpret those clues and talks the defuser through each solution.
The players win by solving all three modules before the countdown reaches zero.

## How a round works

1. **Setup.** Players choose a difficulty and timer length.
   The colored puzzle buttons can double as setup controls.
2. **Start.** Pressing Start locks the settings and begins the countdown.
   The Arduino generates a fresh clue and its corresponding solution for each module.
3. **Play.** A correct solution lights that module's green LED, and the Arduino tracks which modules are complete.
   An incorrect action incurs a penalty.
4. **Finish.** Completing all three modules in time triggers the victory sequence.
   If time expires first, the displays show the loss state and the red warning LED lights.
5. **Reset.** Pressing Reset returns the game to the setup screen for a new randomized round.

The timer and control module shows the countdown on a four-digit display.
The red LED blinks for the low-time warning. Feedback and end states use LEDs and displays only.

## Puzzle modules

### 1. Symbol Sequence

The defuser describes the symbols on the TFT display.
The expert looks them up in the manual and gives the defuser an ordered sequence of colored button presses.
The Arduino checks each press against the solution.

| Difficulty | Clue and response |
| --- | --- |
| Easy | One symbol calls for a simple button response. |
| Medium | Two symbols appear together, and their relationship determines the response. |
| Hard | Four symbols appear in separate sections of the TFT, and the whole arrangement determines a longer button sequence. |

### 2. RGB Flash Sequence

The RGB LED plays a Morse-style flash pattern.
The defuser reports the colors and timing, and the expert decodes the pattern using the manual.
The Arduino checks the defuser's response on the large button.

| Difficulty | Flash pattern |
| --- | --- |
| Easy | One color with simple timing. |
| Medium | Multiple colors with more complex timing. |
| Hard | Both color and timing can vary with every flash, so the defuser must report both for each one. |

### 3. Serial Code

The defuser reads the serial code from the 16×2 LCD.
The expert uses the manual to derive the required settings for the potentiometers and toggle switches.
The Arduino continuously reads the controls and checks the required settings.
Each potentiometer selects a discrete value, such as a digit from 0 to 9.

| Difficulty | Clue and response |
| --- | --- |
| Easy | A short code determines three potentiometer values. One idea is to use properties of a single word. |
| Medium | A more detailed code determines potentiometer values and some switch positions. |
| Hard | Conditional rules applied to a longer alphanumeric code determine all three potentiometer values and all three switch positions. Hexadecimal is one option. |

## Hardware and parts

The Arduino Uno and all four modules share a chassis.
The table includes the part numbers and prices recorded in the original plan.
Ordering status is unknown because the pasted list did not retain its highlighting.

| Assembly | Parts | Sourcing notes |
| --- | --- | --- |
| Shared controller and enclosure | 1 Arduino Uno and a chassis with module faceplates | |
| Puzzle status | 3 green LEDs, one per puzzle module | |
| Symbol Sequence | 1 TFT display and 4 differently colored tall push buttons | TFT: Adafruit 4311, $19.95. Four clicky switches: Adafruit 5123, $3.80. |
| RGB Flash Sequence | 1 RGB LED and 1 large push button | RGB LED: Adafruit 6158, $1.95. |
| Serial Code | 1 16×2 LCD, 1 LCD backpack, 3 potentiometers, and 3 small toggle switches | Backpack: Adafruit 292, $9.95. |
| Timer and controls | 1 four-digit seven-segment display, Start and Reset buttons, 1 red LED, and 1 large master power switch | Display: Adafruit 865, $3.95. |
| Connections and prototyping | 1 QT-to-male connector, an I/O expander, resistors, capacitors, jumper wires, and a breadboard | Connector: $0.95 ([Adafruit 4209](https://www.adafruit.com/product/4209)). Expander options: PCF8574 or Adafruit AW9523 GPIO Expander and LED Driver Breakout (STEMMA QT / Qwiic), $4.95 for the AW9523. |

The controls total seven push buttons (four colored, one large, Start, and Reset) and four switches (three puzzle toggles and the master power switch).

## Assigned design work

The original plan marks all four workstreams below as taken.

### Module 1 and expert manual

- [ ] Design the symbol library and define the button-sequence rules for Easy, Medium, and Hard.
- [ ] Catalog clue types and answer types for the manual.
- [ ] Decide whether to script manual randomization while preserving each clue-to-answer mapping.
- [ ] Check that every possible symbol puzzle has one clear solution.
- [ ] Draft the expert manual and create example puzzles for testing.

### Modules 2 and 3

- [ ] Define the Morse-style encoding, the role of color, and the flash rules for each difficulty.
- [ ] Create example flash sequences and specify how the expert decodes each one into a button response.
- [ ] Explore word-based rules for Easy serial codes, more involved codes for Medium, and alphanumeric or hexadecimal rules for Hard.
- [ ] Map serial codes to the three potentiometers and three switches, and verify that each code has one solution.

### Electronics

- [ ] Finalize the power design, resistor values, and capacitor requirements.
- [ ] Choose an I/O expander and draft the Arduino pin map.
- [ ] Draw the wiring diagram for the TFT, LCD, countdown display, buttons, LEDs, potentiometers, and switches.
- [ ] Test the individual components and identify any additional parts needed.

### Software

- [ ] Set up the Arduino project and implement the game states.
- [ ] Build the setup screen, difficulty and timer selection, and Start and Reset behavior.
- [ ] Implement randomized puzzle generation, module completion tracking, and time penalties.
- [ ] Test individual inputs and outputs.

## Assembly, implementation, and testing

### All three puzzle modules

- [ ] Design each faceplate and mount its controls, display or RGB LED, and green status LED.
- [ ] Wire and test each module's components, including the required LED resistors.
- [ ] Implement Easy, Medium, and Hard using the rules for each module.
- [ ] Handle correct and incorrect inputs, light the green LED on completion, and clear the module state on Reset.

### Symbol Sequence

- [ ] Wire and test the TFT and all four colored buttons.
- [ ] Store the symbol library and answer mappings, then generate and display a random symbol puzzle.
- [ ] Read button presses in order and test every combination against its expected sequence.

### RGB Flash Sequence

- [ ] Wire the RGB LED and large button, then test the colors and button input.
- [ ] Store flash patterns and their answer mappings, then select and play a random pattern with the specified colors and timing.
- [ ] Read and validate the large-button response and test all pattern and response combinations.

### Serial Code

- [ ] Wire and test the LCD, three potentiometers, and three toggle switches.
- [ ] Generate and display random serial codes using the serial-to-settings rules.
- [ ] Convert potentiometer readings into discrete values, read the switch positions, and check the required configuration.
- [ ] Test randomized puzzles against their expected settings.

### Timer and controls

- [ ] Mount the countdown display, Start and Reset buttons, red LED, and master switch.
- [ ] Wire and test the display, buttons, and red LED.
- [ ] Connect setup selections to the countdown, lock them on Start, and return to setup on Reset.
- [ ] Implement low-time warnings, the victory sequence, and the simulated explosion sequence.
- [ ] Test all timer settings and game modes.

## Design references

- **AoE Mockup** illustrates the intended physical layout.
- **PDF, page 7** is the visual reference for the Hard symbol puzzle.

The pasted plan does not include a link to either reference.
