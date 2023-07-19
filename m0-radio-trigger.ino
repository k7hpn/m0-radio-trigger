// m0-radio-trigger.ino
// Adafruit Feather M0 Radio with RFM69 Trigger

// for details and license information see
// https://github.com/k7hpn/m0-radio-trigger

#include <SPI.h>

#include <RH_RF69.h>

/** Trigger configurables **/

// pin to read battery voltage from, taken from Adafruit docs
//#define TRIGGER_BATTERY_PIN A7

// pin that triggers S when grounded
#define TRIGGER_S 5

// pins to set for L298N upon S trigger
#define S_OUTPUT_L1 D24
#define S_OUTPUT_L2 D25
#define S_OUTPUT_R1 0
#define S_OUTPUT_R2 1

// pins that trigger wings when grounded
#define TRIGGER_WING_IN 10
#define TRIGGER_WING_OUT 11
#define TRIGGER_WING_KILL 12

// pins to set for L298N wing triggersz
#define W_OUTPUT_L1 A0
#define W_OUTPUT_L2 A1
#define W_OUTPUT_R1 A2
#define W_OUTPUT_R2 A3

// when the battery is low, flash the on-board LED 4 times this often (in ms)
const unsigned long batteryStatusDelay = 5000;
// how long to wait after S button press is read before triggering
const unsigned long triggerDelay = 2000;
// the duration S trigger lasts
const unsigned long triggerDuration = 4000;
// how long to kill wings after an in or an out
const unsigned long wingsKillDelay = 15000;
// whether or not to light up the on-board LED
const bool enableLed = true;

// things you probably won't have to change

// checks the state of things this frequently
const unsigned long checkStatus = 1000;
// the delay in ms for switch debouncing
const unsigned long debounceDelay = 50;

// encrypted text for wireless triggering
const char triggerText[5] = "GOSM";
const char wingsOutText[5] = "GOWO";
const char wingsInText[5] = "GOWI";
const char wingsKillText[5] = "GOWK";

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

#if defined(ARDUINO_ADAFRUIT_FEATHER_RP2040_RFM)  // Feather RP2040 w/Radio
  #define RFM69_CS   16
  #define RFM69_INT  21
  #define RFM69_RST  17
  #define LED        LED_BUILTIN
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

  pinMode(TRIGGER_S, INPUT_PULLUP);
  pinMode(TRIGGER_WING_OUT, INPUT_PULLUP);
  pinMode(TRIGGER_WING_IN, INPUT_PULLUP);
  pinMode(TRIGGER_WING_KILL, INPUT_PULLUP);

  pinMode(S_OUTPUT_L1, OUTPUT);
  pinMode(S_OUTPUT_L2, OUTPUT);
  pinMode(S_OUTPUT_R1, OUTPUT);
  pinMode(S_OUTPUT_R2, OUTPUT);

  pinMode(W_OUTPUT_L1, OUTPUT);
  pinMode(W_OUTPUT_L2, OUTPUT);
  pinMode(W_OUTPUT_R1, OUTPUT);
  pinMode(W_OUTPUT_R2, OUTPUT);

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

// this will be true when we're latched into a S button state to avoid multiple triggers
bool latchS = false;

// the true value of the button once we've debounced it
int debouncedS = HIGH;
int debouncedWO = HIGH;
int debouncedWI = HIGH;
int debouncedWK = HIGH;

// the value of millis() when we last checked the battery status, 0 means we have no status
unsigned long batteryTime = 0;
// the value of millis() when S last triggered, 0 means we're not currently triggered
unsigned long lastTriggerSTime = 0;
// the value of millis() last time we did a status check
unsigned long timer = 0;
// the value of millis() when we should turn off the trigger, 0 means we're not pending a turn off
unsigned long triggerOff = 0;
// the scheduled time for when to execute a wings kill
unsigned long scheduleWingsKill = 0;

// the value of millis() when we last started a debounce for each button
unsigned long lastDebounceSTime = 0;
unsigned long lastDebounceWOTime = 0;
unsigned long lastDebounceWITime = 0;
unsigned long lastDebounceWKTime = 0;

void loop()
{
  // read button statuses
  int checkS = digitalRead(TRIGGER_S);
  int checkWO = digitalRead(TRIGGER_WING_OUT);
  int checkWI = digitalRead(TRIGGER_WING_IN);
  int checkWK = digitalRead(TRIGGER_WING_KILL);

  // once the S button has been officially un-pressed we can unlatch
  if (debouncedS == HIGH)
  {
    latchS = false;
  }

  // once checkS has been in state for debounceDelay ms we can officially change debouncedS
  if ((millis() - lastDebounceSTime) > debounceDelay)
  {
    if (checkS != debouncedS)
    {
      debouncedS = checkS;
    }

    // debounced button value while we are not latched means we've been triggered
    if (debouncedS == LOW && !latchS)
    {
      lastTriggerSTime = millis();
      Blink(LED, 60, 1);
      // send a message to any other listening triggers
      Serial.println("S triggered");
      rf69.send((uint8_t *)triggerText, strlen(triggerText));
      rf69.waitPacketSent();
      // set the latchS so each button press results in only one signal to trigger
      latchS = true;
    }
  }

  if ((millis() - lastDebounceWOTime) > debounceDelay)
  {
    if (checkWO != debouncedWO)
    {
      debouncedWO = checkWO;
    }

    // debounced official button value while we are not latchSed means we've been triggered
    if (debouncedWO == LOW)
    {
      WingsOut();
      Blink(LED, 60, 1);
      Serial.println("Wings out triggered");
      debouncedWO = HIGH;
      // send a message to any other listening triggers
      rf69.send((uint8_t *)wingsOutText, strlen(wingsOutText));
      rf69.waitPacketSent();
    }
  }

  if ((millis() - lastDebounceWITime) > debounceDelay)
  {
    if (checkWI != debouncedWI)
    {
      debouncedWI = checkWI;
    }

    // debounced official button value while we are not latchSed means we've been triggered
    if (debouncedWI == LOW)
    {
      WingsIn();
      Blink(LED, 60, 1);
      Serial.println("Wings in triggered");
      debouncedWI = HIGH;
      // send a message to any other listening triggers
      rf69.send((uint8_t *)wingsInText, strlen(wingsInText));
      rf69.waitPacketSent();
    }
  }

  if ((millis() - lastDebounceWKTime) > debounceDelay)
  {
    if (checkWK != debouncedWK)
    {
      debouncedWK = checkWK;
    }

    // debounced official button value while we are not latchSed means we've been triggered
    if (debouncedWK == LOW)
    {
      WingsKill();
      Blink(LED, 60, 1);
      Serial.println("Wings kill triggered");
      debouncedWK = HIGH;
      // send a message to any other listening triggers
      rf69.send((uint8_t *)wingsKillText, strlen(wingsKillText));
      rf69.waitPacketSent();
    }
  }

  // every checkStatus ms we do an evaluation of what's going on
  if (millis() - timer > checkStatus)
  {
    if (triggerOff > 0 && millis() > triggerOff)
    {
      Serial.println("S off");
      triggerOff = 0;
      digitalWrite(S_OUTPUT_L1, LOW);
      digitalWrite(S_OUTPUT_L2, LOW);
      digitalWrite(S_OUTPUT_R1, LOW);
      digitalWrite(S_OUTPUT_R2, LOW);
    }

    if (scheduleWingsKill > 0 && millis() > scheduleWingsKill)
    {
      Serial.print("Timer-fired wings kill");
      scheduleWingsKill = 0;
      WingsKill();
      Blink(LED, 200, 2);
    }

    // read current battery voltage value - logic from Adafruit link (see above)
    //float batteryVoltage = analogRead(TRIGGER_BATTERY_PIN) * 6.6 / 1024;
    // if the battery value < 3.6 volts then we are in a low battery state
    //if (batteryTime < millis() && batteryVoltage < 3.6)
    //{
    //  Blink(LED, 250, 4);
    //  batteryTime = millis() + batteryStatusDelay;
    //}

    // reset the clock for our next status update
    timer = millis();
  }

  // if we are at lastTriggerSTime + triggerDelay then it's time to turn on the trigger
  if (lastTriggerSTime > 0 && (lastTriggerSTime + triggerDelay) < millis())
  {
    // digitalWrite(TRIGGER_OUTPUT_PIN, HIGH);
    triggerOff = millis() + triggerDuration;
    // Two long blinks = trigger going
    Serial.println("S active");
    digitalWrite(S_OUTPUT_L1, LOW);
    digitalWrite(S_OUTPUT_L2, HIGH);
    digitalWrite(S_OUTPUT_R1, LOW);
    digitalWrite(S_OUTPUT_R2, HIGH);
    Blink(LED, 200, 2);
    lastTriggerSTime = 0;
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
        lastTriggerSTime = millis();
        Serial.print("At time: ");
        Serial.print(lastTriggerSTime);
        Serial.print(" radio trigger S received, will trigger at: ");
        Serial.print(lastTriggerSTime + triggerDelay);
        Serial.print(" RSSI: ");
        Serial.println(rf69.lastRssi(), DEC);
      }
      if (strstr((char *)buf, wingsOutText))
      {
        WingsOut();
        Blink(LED, 30, 2); // two blinks for activation via wireless
        Serial.print("Radio request for wings out, RSSI: ");
        Serial.println(rf69.lastRssi(), DEC);
      }
      if (strstr((char *)buf, wingsInText))
      {
        WingsIn();
        Blink(LED, 30, 2); // two blinks for activation via wireless
        Serial.print("Radio request for wings in, RSSI: ");
        Serial.println(rf69.lastRssi(), DEC);
      }
      if (strstr((char *)buf, wingsKillText))
      {
        WingsKill();
        Blink(LED, 30, 2); // two blinks for activation via wireless
        Serial.print("Radio request for wings kill, RSSI: ");
        Serial.println(rf69.lastRssi(), DEC);
      }
    }
    else
    {
      Serial.println("Receive failed");
    }
  }
}

void WingsIn()
{
  digitalWrite(W_OUTPUT_L1, LOW);
  digitalWrite(W_OUTPUT_L2, HIGH);
  digitalWrite(W_OUTPUT_R1, LOW);
  digitalWrite(W_OUTPUT_R2, HIGH);
  scheduleWingsKill = millis() + wingsKillDelay;
}
void WingsOut()
{
  digitalWrite(W_OUTPUT_L1, HIGH);
  digitalWrite(W_OUTPUT_L2, LOW);
  digitalWrite(W_OUTPUT_R1, HIGH);
  digitalWrite(W_OUTPUT_R2, LOW);
  scheduleWingsKill = millis() + wingsKillDelay;
}
void WingsKill()
{
  digitalWrite(W_OUTPUT_L1, LOW);
  digitalWrite(W_OUTPUT_L2, LOW);
  digitalWrite(W_OUTPUT_R1, LOW);
  digitalWrite(W_OUTPUT_R2, LOW);
  scheduleWingsKill = 0;
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
