# m0-radio-trigger

Use [Adafruit Feather M0 RFM69](https://learn.adafruit.com/adafruit-feather-m0-radio-with-rfm69-packet-radio) boards with attached buttons to trigger attached L298N motor drivers as well as L298Ns attached to other boards nearby with the correct wireless configuration. Everyboard is a sender and a receiver of triggers.

- Set a unique `key[]` value for encryption between the systems.

- Trigger S

  1. Triggered by grounding pin `TRIGGER_S` (pin 5 by default)
  2. Transmits a signal to start this process on all listening devices and does it locally
  3. Waits `triggerDelay` ms (10 seconds by default)
  4. Sets `S_OUTPUT_L1` and `S_OUTPUT_R1` low (pins A4 and 0) and `S_OUTPUT_L2` and `S_OUTPUT_R2` high (pins A5 and 1)
  5. Waits `triggerDuration` ms (2 seconds)
  6. Sets `S_OUTPUT_L1`, `S_OUTPUT_R1`, `S_OUTPUT_L2`, and `S_OUTPUT_R2` low

- Trigger Wings

  - Grounding pin `TRIGGER_WING_IN` (10) sets `W_OUTPUT_L1` (A0) and `W_OUTPUT_R1` (A2) low, `W_OUTPUT_L2` (A1) and `W_OUTPUT_R2` (A3) high, and sends a message to other boards to do the same
  - Grounding pin `TRIGGER_WING_OUT` (11) sets `W_OUTPUT_L1` and `W_OUTPUT_R1` high, `W_OUTPUT_L2` and `W_OUTPUT_R2` high, and sends a message to other boards to do the same
  - Grounding pin `TRIGGER_WING_KILL` (12) sets `W_OUTPUT_L1`, `W_OUTPUT_L2`, `W_OUTPUT_R1`, and `W_OUTPUT_R2` low
  - Either of the first two actions above schedules the `TRIGGER_WING_KILL` action after `wingsKillDelay` ms (15 seconds)

- On-board LED

  - Disable `enableLed` if necessary to save power.
  - One short blink - we've scheduled a trigger from button input
  - Two short blinks - we've scheduled a trigger from wireless input
  - Two long blinks - a scheduled trigger is being fired
  - Four long blinks - battery voltage is low

Much of this code came from:

- The [RadioHead Packet Radio library](http://www.airspayce.com/mikem/arduino/RadioHead/) with [modifications by adafruit](https://github.com/adafruit/RadioHead)
- [Debounce on a pushbutton](https://docs.arduino.cc/built-in-examples/digital/Debounce) from the Arduino docs

## License

The RF code used in this project is derived from the [adafruit/RadioHead](https://github.com/adafruit/RadioHead) project on GitHub. It carries a GPL V2 license so this code does as well.
