// m0-radio-trigger.ino
// Adafruit Feather M0 Radio with RFM69 Trigger

// for details and license information see
// https://github.com/k7hpn/m0-radio-trigger

#include <SPI.h>

#include <RH_RF69.h>

/** Trigger configurables **/

// pin to read battery voltage from, taken from Adafruit docs
#define TRIGGER_BATTERY_PIN A7
// pin to monitor for button presses
#define TRIGGER_BUTTON_PIN 5
// set this pin high for the duration of the trigger
#define TRIGGER_OUTPUT_PIN 10

// when the battery is low, flash the on-board LED 4 times this often (in ms)
const unsigned long batteryStatusDelay = 5000;
// how long to wait after a button press is read before triggering
const unsigned long triggerDelay = 10000;
// the duration a trigger lasts
const unsigned long triggerDuration = 2000;
// whether or not to light up the on-board LED
const bool enableLed = true;

// things you probably won't have to change

// check the state of things this frequently
const unsigned long checkStatus = 1000;
// the delay in ms for switch debouncing
const unsigned long debounceDelay = 50;
// encrypted text for wireless triggering
const char triggerText[3] = "GO";

/************ Radio Setup ***************/

// Change to 434.0 or other frequency, must match RX's freq!
#define RF69_FREQ 915.0

#if defined(__AVR_ATmega32U4__) // Feather 32u4 w/Radio
#define RFM69_CS 8
#define RFM69_INT 7
#define RFM69_RST 4
#define LED 13
#endif

#if defined(ADAFRUIT_FEATHER_M0) || defined(ADAFRUIT_FEATHER_M0_EXPRESS) || defined(ARDUINO_SAMD_FEATHER_M0)
// Feather M0 w/Radio
#define RFM69_CS 8
#define RFM69_INT 3
#define RFM69_RST 4
#define LED 13
#endif

#if defined(__AVR_ATmega328P__) // Feather 328P w/wing
#define RFM69_INT 3             //
#define RFM69_CS 4              //
#define RFM69_RST 2             // "A"
#define LED 13
#endif

#if defined(ESP8266) // ESP8266 feather w/wing
#define RFM69_CS 2   // "E"
#define RFM69_IRQ 15 // "B"
#define RFM69_RST 16 // "D"
#define LED 0
#endif

#if defined(ARDUINO_ADAFRUIT_FEATHER_ESP32S2) || defined(ARDUINO_NRF52840_FEATHER) || defined(ARDUINO_NRF52840_FEATHER_SENSE)
#define RFM69_INT 9  // "A"
#define RFM69_CS 10  // "B"
#define RFM69_RST 11 // "C"
#define LED 13

#elif defined(ESP32) // ESP32 feather w/wing
#define RFM69_RST 13 // same as LED
#define RFM69_CS 33  // "B"
#define RFM69_INT 27 // "A"
#define LED 13
#endif

#if defined(ARDUINO_NRF52832_FEATHER)
/* nRF52832 feather w/wing */
#define RFM69_RST 7  // "A"
#define RFM69_CS 11  // "B"
#define RFM69_INT 31 // "C"
#define LED 17
#endif

// Singleton instance of the radio driver
RH_RF69 rf69(RFM69_CS, RFM69_INT);

void setup()
{
  Serial.begin(115200);
  // uncomment below if you want to see setup() console output
  // while (!Serial) { delay(1); }

  pinMode(TRIGGER_BUTTON_PIN, INPUT_PULLUP);
  pinMode(TRIGGER_OUTPUT_PIN, OUTPUT);
  pinMode(LED, OUTPUT);

  pinMode(RFM69_RST, OUTPUT);
  digitalWrite(RFM69_RST, LOW);

  Serial.println("Trigger startup");
  Serial.println();

  // manual reset
  digitalWrite(RFM69_RST, HIGH);
  delay(10);
  digitalWrite(RFM69_RST, LOW);
  delay(10);

  if (!rf69.init())
  {
    Serial.println("RFM69 radio init failed");
    while (1)
      ;
  }
  Serial.println("RFM69 radio init OK!");

  // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM (for low power module)
  // No encryption
  if (!rf69.setFrequency(RF69_FREQ))
  {
    Serial.println("setFrequency failed");
  }

  // If you are using a high power RF69 eg RFM69HW, you *must* set a Tx power with the
  // ishighpowermodule flag set like this:
  rf69.setTxPower(20, true); // range from 14-20 for power, 2nd arg must be true for 69HCW

  // The encryption key has to be the same as the one in the server
  uint8_t key[] = {
      0x01,
      0x02,
      0x03,
      0x04,
      0x05,
      0x06,
      0x07,
      0x08,
      0x01,
      0x02,
      0x03,
      0x04,
      0x05,
      0x06,
      0x07,
      0x08};
  rf69.setEncryptionKey(key);

  pinMode(LED, OUTPUT);

  Serial.print("RFM69 radio @");
  Serial.print((int)RF69_FREQ);
  Serial.println(" MHz");
}

// this will be true when we're latched into a button state to avoid multiple triggers
bool latch = false;
// the true value of the button once we've debounced it
int debouncedButtonValue = HIGH;
// the value of millis() when we last checked the battery status, 0 means we have no status
unsigned long batteryTime = 0;
// the value of millis() when we were last triggered, 0 means we're not currently triggered
unsigned long lastTriggerTime = 0;
// the value of millis() last time we did a status check
unsigned long timer = 0;
// the value of millis() when we should turn off the trigger, 0 means we're not pending a turn off
unsigned long triggerOff = 0;
// the value of millis() when we last started a debounce
unsigned long lastDebounceTime = 0;

void loop()
{
  // read button status
  int check = digitalRead(TRIGGER_BUTTON_PIN);

  // once the button has been officially un-pressed we can unlatch
  if (debouncedButtonValue == HIGH)
  {
    latch = false;
  }

  // once check has been in state for debounceDelay ms we can officially change debouncedButtonValue
  if ((millis() - lastDebounceTime) > debounceDelay)
  {
    if (check != debouncedButtonValue)
    {
      debouncedButtonValue = check;
    }

    // debounced official button value while we are not latched means we've been triggered
    if (debouncedButtonValue == LOW && !latch)
    {
      lastTriggerTime = millis();
      Serial.print("At time: ");
      Serial.print(lastTriggerTime);
      Serial.print(" sending message and scheduling local trigger for: ");
      Serial.println(lastTriggerTime + triggerDelay);
      // blink one time to acknowledge
      Blink(LED, 60, 1);
      // send a message to any other listening triggers
      rf69.send((uint8_t *)triggerText, strlen(triggerText));
      rf69.waitPacketSent();
      // set the latch so each button press results in only one signal to trigger
      latch = true;
    }
  }

  // every checkStatus ms we do an evaluation of what's going on
  if (millis() - timer > checkStatus)
  {
    if (lastTriggerTime > 0)
    {
      // we have a pending trigger, write out for debugging purposes
      Serial.print("At time: ");
      Serial.print(millis());
      Serial.print(", last trigger recorded: ");
      Serial.print(lastTriggerTime);
      Serial.print(", trigger fires at: ");
      Serial.println(lastTriggerTime + triggerDelay);
    }
    if (triggerOff > 0 && millis() > triggerOff)
    {
      // we've been triggered for triggerOff ms, time to un-trigger
      Serial.print("At time: ");
      Serial.print(millis());
      Serial.println(" turning the trigger off.");
      triggerOff = 0;
      digitalWrite(TRIGGER_OUTPUT_PIN, LOW);
    }
    // read current battery voltage value - logic from Adafruit link (see above)
    float batteryVoltage = analogRead(TRIGGER_BATTERY_PIN) * 6.6 / 1024;
    // if the battery value < 3.6 volts then we are in a low battery state
    if (batteryTime < millis() && batteryVoltage < 3.6)
    {
      Blink(LED, 250, 4);
      batteryTime = millis() + batteryStatusDelay;
    }
    // reset the clock for our next status update
    timer = millis();
  }

  // if we are at lastTriggerTime + triggerDelay then it's time to turn on the trigger
  if (lastTriggerTime > 0 && (lastTriggerTime + triggerDelay) < millis())
  {
    digitalWrite(TRIGGER_OUTPUT_PIN, HIGH);
    triggerOff = millis() + triggerDuration;
    Serial.print("At time: ");
    Serial.print(millis());
    Serial.print(" it is trigger time: ");
    Serial.print(lastTriggerTime + triggerDelay);
    Serial.print(" turning off at: ");
    Serial.println(triggerOff);
    // Two long blinks = trigger going
    Blink(LED, 200, 2);
    lastTriggerTime = 0;
  }

  if (rf69.available())
  {
    // Should be a message for us now
    uint8_t buf[RH_RF69_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);
    if (rf69.recv(buf, &len))
    {
      if (!len)
        return;
      buf[len] = 0;
      if (strstr((char *)buf, triggerText))
      {
        Blink(LED, 30, 2); // two blinks for schedule via wireless
        lastTriggerTime = millis();
        Serial.print("At time: ");
        Serial.print(lastTriggerTime);
        Serial.print(" radio trigger received, will trigger at: ");
        Serial.print(lastTriggerTime + triggerDelay);
        Serial.print(" RSSI: ");
        Serial.println(rf69.lastRssi(), DEC);
      }
    }
    else
    {
      Serial.println("Receive failed");
    }
  }
}

void Blink(byte PIN, byte DELAY_MS, byte loops)
{
  if (enableLed == false)
  {
    return;
  }

  for (byte i = 0; i < loops; i++)
  {
    if (i > 0)
    {
      delay(DELAY_MS);
    }
    digitalWrite(PIN, HIGH);
    delay(DELAY_MS);
    digitalWrite(PIN, LOW);
  }
}
