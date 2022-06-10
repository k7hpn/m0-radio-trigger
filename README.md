# m0-radio-trigger

Use [Adafruit Feather M0 RFM69](https://learn.adafruit.com/adafruit-feather-m0-radio-with-rfm69-packet-radio) Arduino boards with attached buttons to trigger a GPIO port on that board as well as other boards nearby with the correct wireless configuration.

- Most configurables are set in preprocessor directives and constants at the top.
- Wire up a button which connects `TRIGGER_BUTTON_PIN` to ground.
- Wire up your item to be triggered to `TRIGGER_OUTPUT_PIN`.
- Set a unique `key[]` value for encryption between the systems.
- Set `triggerDelay` to the number of milliseconds to delay after a button is pressed.
- Adjust `triggerDuration` to how long you want the `TRIGGER_OUTPUT_PIN` to stay on when triggered.
- Disable `enableLed` if necessary to save power.

If the LED is enabled, here are some things you might see:

- One short blink - we've scheduled a trigger from button input
- Two short blinks - we've scheduled a trigger from wireless input
- Two long blinks - the trigger is being fired
- Four long blinks - battery voltage is low

Much of this code came from:

- The [RadioHead Packet Radio library](http://www.airspayce.com/mikem/arduino/RadioHead/) with [modifications by adafruit](https://github.com/adafruit/RadioHead)
- [Debounce on a pushbutton](https://docs.arduino.cc/built-in-examples/digital/Debounce) from the Arduino docs

## License

The RF code used in this project is derived from the [adafruit/RadioHead](https://github.com/adafruit/RadioHead) project on GitHub. It carries a GPL V2 license so this code does as well.
