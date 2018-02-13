#include <Arduino.h>
#include <HardwareSerial.h>
#include <TimerOne.h>
#include <Matrix.h>

extern HardwareSerial Serial;

//                                  +-----+
//                     +------------| USB |-------------+
//                     |            +-----+             |
// arcadeBtnsDin[2] -> | [ ]D13/SCK         MISO/D12[ ] | -> arcadeBtnsLeds[2]
//                     | [ ]3.3V            MOSI/D11[ ]~| <- arcadeBtnsDin[1]
//                     | [ ]V.ref     ___     SS/D10[ ]~| -> arcadeBtnsLeds[1]
// PRESSURE_LED_PIN <- | [ ]A0       / N \        D9[ ]~| <- arcadeBtnsDin[0]
// PRESSURE_BTN_PIN -> | [ ]A1      /  A  \       D8[ ] | -> arcadeBtnsLeds[0]
// MATRIX_LED_PIN   <- | [ ]A2      \  N  /       D7[ ] | <- switchboardPins[5]
//                     | [ ]A3       \_0_/        D6[ ]~| <- switchboardPins[4]
//     potiPins[0]  -> | [ ]A4/SDA                D5[ ]~| <- switchboardPins[3]
//     potiPins[1]  -> | [ ]A5/SCL                D4[ ] | <- switchboardPins[2]
//     potiPins[2]  -> | [ ]A6 (noDIn)       INT1/D3[ ]~| <- switchboardPins[1]
//     potiPins[3]  -> | [ ]A7 (noDIn)       INT0/D2[ ] | <- switchboardPins[0]
//                     | [ ]5V                   GND[ ] |
//                     | [ ]RST                  RST[ ] |
//                     | [ ]GND   5V MOSI GND D0/RX1[ ] |
//                     | [ ]Vin   [ ] [ ] [ ] D1/TX1[ ] |
//                     |          [ ] [ ] [ ]           |
//                     |          MISO SCK RST          |
//                     | NANO-V3                        |
//                     +--------------------------------+
                   
// Common
#define LED_BRIGHTNESS 5

// Switchboard
#define SWITCHBOARD_NUM_PINS 6
const int switchboardPins[] = { 2, 3, 4, 5, 6, 7 };
int switchboardActiveOut = 0;
int switchboardActiveIn = 0;
Matrix matrix = Matrix();

// *** MATRIX ***
void setupMatrix() {
  matrix.show('a');
}

// *** MATRIX ***
void loopMatrix() {
  matrix.update();
}

// SWITCHBOARD
void setupSwitchboard() {
  // Reset all Pins as INPUTs
  for (int i=0; i < SWITCHBOARD_NUM_PINS; i++) {
    pinMode(switchboardPins[i], INPUT_PULLUP);
  }
  switchboardActiveOut = 0;
  switchboardActiveIn = 0;
}

// SWITCHBOARD
void loopSwitchboard() {
  // Reset
  int inputPinReadout = LOW;
  setupSwitchboard();

  // Loop all pins switching one as OUTPUT
  for(int outputPinIndex=0; outputPinIndex < SWITCHBOARD_NUM_PINS && switchboardActiveIn == 0; outputPinIndex++) {

    // set the output pin LOW
    pinMode(switchboardPins[outputPinIndex], OUTPUT);
    digitalWrite(switchboardPins[outputPinIndex], LOW);

    for(int inputPinIndex=outputPinIndex+1; inputPinIndex < SWITCHBOARD_NUM_PINS && switchboardActiveIn == 0; inputPinIndex++) {

      // Don't switch to INPUT for the pin that is currently OUTPUT
      if(inputPinIndex == outputPinIndex) {
        continue;
      }
      pinMode(switchboardPins[inputPinIndex], INPUT_PULLUP);

      inputPinReadout = digitalRead(switchboardPins[inputPinIndex]);

      if (inputPinReadout == LOW) {
        switchboardActiveIn = inputPinIndex;
        switchboardActiveOut = outputPinIndex;
        #ifdef DEBUG
          Serial.print("[SWITCHBOARD] Connection from IN ");
          Serial.print(switchboardActiveIn);
          Serial.print(" (");
          Serial.print(switchboardPins[switchboardActiveIn]);
          Serial.print(") to OUT ");
          Serial.print(switchboardActiveOut);
          Serial.print(" (");
          Serial.print(switchboardPins[switchboardActiveOut]);
          Serial.println(")");
        #endif
      }
    }

    // Set last loop pin to INPUT again
    pinMode(switchboardPins[outputPinIndex], INPUT_PULLUP);
  }
}

void setup()
{
  Serial.begin(9600);

  setupMatrix();
  setupSwitchboard();
}

void loop()
{
  loopMatrix();
  loopSwitchboard();
}
