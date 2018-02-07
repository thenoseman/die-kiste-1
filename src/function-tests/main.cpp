#include <Arduino.h>
#include <FastLED.h>
#include <HardwareSerial.h>
#include <TimerOne.h>

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

// Matrix display
const uint8_t MATRIX_LED_PIN = A2;
#define MATRIX_NUM_LEDS 100
#define MATRIX_DIM 10
#define MATRIX_LED_TYPE NEOPIXEL
CRGB matrixLeds[MATRIX_NUM_LEDS];

// Potentiometers
#define POTIS_NUM_POTIS 4
const int potiPins[] = {4,5,6,7};
int potiActiveValues[] = {0,0,0,0};
int potiActiveIndex = 0;

// Arcade Switches
#define AS_NUM_SWITCHES 3
const int arcadeBtnsDin[] = {9, 11, 13};
const int arcadeBtnsLeds[] = {8, 10, 12};

// Pressure Gauge
#define PRESSURE_LED_PIN A0
#define PRESSURE_BTN_PIN A1
#define PRESSURE_NUM_LEDS 5
#define PRESSURE_LED_TYPE NEOPIXEL
#define PRESSURE_TIMER_MICRO 100000 // 100 millisecs
CRGB pressureLeds[PRESSURE_NUM_LEDS];
volatile int pressurePercent = 0;
// These colors are used indicating the "danger" level, starting at 0 (no led lite) to 5 (all lite)
const long unsigned int pressureColors[] = {CRGB::Black, CRGB::LawnGreen, CRGB::Green, CRGB::Yellow, CRGB::Orange, CRGB::Red};

// Switchboard
#define SWITCHBOARD_NUM_PINS 6
const int switchboardPins[] = { 2, 3, 4, 5, 6, 7 };
int switchboardActiveOut = 0;
int switchboardActiveIn = 0;

// LED Numbers and letters
#define MATRIX_ALPHABET_SIZE 36
#define MATRIX_ALPHABET_L_SIZE 13
const int matrixAlphabet[MATRIX_ALPHABET_SIZE][MATRIX_ALPHABET_L_SIZE] = {
  {10,11,12,13,20,24,31,32,33,34,-1,-1,-1}, // 0
  {13,20,21,22,23,24,-1,-1,-1,-1,-1,-1,-1}, // 1
  {10,11,14,20,22,24,30,33,-1,-1,-1,-1,-1}, // 2
  {10,14,20,22,24,31,33,-1,-1,-1,-1,-1,-1}, // 3
  {12,13,14,22,30,31,32,33,34,-1,-1,-1,-1}, // 4
  {10,12,13,14,20,22,24,31,34,-1,-1,-1,-1}, // 5
  {10,11,12,13,20,22,24,30,31,32,34,-1,-1}, // 6
  {10,11,14,22,24,33,34,-1,-1,-1,-1,-1,-1}, // 7
  {10,11,12,13,14,20,22,24,30,31,32,33,34}, // 8
  {10,12,13,14,20,22,24,31,32,33,34,-1,-1}, // 9
  {10,11,12,13,22,24,30,31,32,33,-1,-1,-1}, // A
  {10,11,12,13,14,20,22,24,31,33,-1,-1,-1}, // B
  {11,12,13,20,24,30,34,-1,-1,-1,-1,-1,-1}, // C
  {10,11,12,13,14,20,24,31,32,33,-1,-1,-1}, // D
  {10,11,12,13,14,20,22,24,30,32,34,-1,-1}, // E
  {10,11,12,13,14,22,24,32,34,-1,-1,-1,-1}, // F
  {11,12,13,20,22,24,30,31,32,34,-1,-1,-1}, // G
  {10,11,12,13,14,22,30,31,32,33,34,-1,-1}, // H
  {10,14,20,21,22,23,24,30,34,-1,-1,-1,-1}, // I
  {11,20,31,32,33,34,-1,-1,-1,-1,-1,-1,-1}, // J
  {10,11,12,13,14,22,30,31,33,34,-1,-1,-1}, // K
  {10,11,12,13,14,20,30,-1,-1,-1,-1,-1,-1}, // L
  {10,11,12,13,14,22,23,30,31,32,33,34,-1}, // M
  {10,11,12,13,14,21,22,23,30,31,32,33,34}, // N
  {11,12,13,20,24,31,32,33,-1,-1,-1,-1,-1}, // O
  {10,11,12,13,14,22,24,33,-1,-1,-1,-1,-1}, // P
  {11,12,13,20,21,24,30,31,32,33,-1,-1,-1}, // Q
  {10,11,12,13,14,21,22,24,30,32,33,-1,-1}, // R
  {10,13,20,22,24,31,34,-1,-1,-1,-1,-1,-1}, // S
  {14,20,21,22,23,24,34,-1,-1,-1,-1,-1,-1}, // T
  {11,12,13,14,20,30,31,32,33,34,-1,-1,-1}, // U
  {12,13,14,20,21,32,33,34,-1,-1,-1,-1,-1}, // V
  {10,11,12,13,14,21,22,30,31,32,33,34,-1}, // W
  {10,11,13,14,22,30,31,33,34,-1,-1,-1,-1}, // X
  {13,14,20,21,22,33,34,-1,-1,-1,-1,-1,-1}, // Y
  {10,11,14,20,22,24,30,33,34,-1,-1,-1,-1}  // Z
};
int currentLedNumber = 0;
int currentLedLetterIndex = 10;
int currentLedNumberStartColumn = 4;
unsigned long currentLedNumberDurationMillis = 1000;
unsigned long currentLedNumberStartMillis = 0;

// *** MATRIX ***
void setupMatrix() {
  FastLED.addLeds<MATRIX_LED_TYPE, MATRIX_LED_PIN>(matrixLeds, MATRIX_NUM_LEDS);
}

// Displays a number in a column using a CRGB::.... color
void matrixShowNumber(int index, int startColumn, int startRow, unsigned int color) {
  for(int i=0; i < MATRIX_ALPHABET_L_SIZE ; i++) {
    int currentLedPos = matrixAlphabet[index][i];

    if(currentLedPos == -1) {
      break;
    }

    matrixLeds[currentLedPos + MATRIX_DIM * startColumn + startRow] = color;
  }
}

// *** MATRIX ***
void loopMatrix() {

  // Loop setting potis:
  for (unsigned int potiIndex = 0; potiIndex < 4; potiIndex++) {
    int mStart = potiIndex * 10;
    int mEnd = mStart + 10 - potiActiveValues[potiIndex];

    for (int i = mStart; i < (mStart + 10); i++) {
      matrixLeds[i] = CRGB::Yellow;
    }

    for (int i = mStart; i < mEnd; i++) {
      matrixLeds[i] = CRGB::Red;
    }
  }

  if(millis() > (currentLedNumberStartMillis + currentLedNumberDurationMillis)) {

    // Show numbers
    int prevNumber = (currentLedNumber - 1) < 0 ? 9 : (currentLedNumber - 1);
    matrixShowNumber(prevNumber, currentLedNumberStartColumn, 0, CRGB::Black);
    matrixShowNumber(currentLedNumber, currentLedNumberStartColumn, 0, CRGB::Blue);

    currentLedNumber++;
    if(currentLedNumber > 9) {
      currentLedNumber = 0;
    }

    // Show letters
    int prevLetterIndex = (currentLedLetterIndex - 1) < 0 ? MATRIX_ALPHABET_SIZE - 1 : (currentLedLetterIndex - 1);
    matrixShowNumber(prevLetterIndex, currentLedNumberStartColumn + 1, 5, CRGB::Black);
    matrixShowNumber(currentLedLetterIndex, currentLedNumberStartColumn + 1, 5, CRGB::DarkOrange);
    currentLedLetterIndex++;
    if(currentLedLetterIndex >= MATRIX_ALPHABET_SIZE) {
      currentLedLetterIndex = 10;
    }

    currentLedNumberStartMillis = millis();
  }

}

// *** POTENTIOMETER ***
void setupPotis() {
}

// *** POTENTIOMETER ***
void loopPotis() {

  for (int potiIndex=0; potiIndex < POTIS_NUM_POTIS; potiIndex++) {
    int potiValue = constrain(map(analogRead(potiPins[potiIndex]), 0, 1023, 0, 10), 0, 9);

    // If one change is found, skill polling of other potentiometers
    // TODO: remove bouncing 0->1->0 effect
    if (potiValue != potiActiveValues[potiIndex]) {
      #ifdef DEBUG
        Serial.print("[POTENTIOMETER] #");
        Serial.print(potiIndex);
        Serial.print(" on A");
        Serial.print(potiPins[potiIndex]);
        Serial.print(" = ");
        Serial.println(potiValue);
      #endif

      // This will be picked up by loopMatrix()
      potiActiveValues[potiIndex] = potiValue;
      potiActiveIndex = potiIndex;
    }
  }
}

// ARCADE SWITCHES
void setupArcadeSwitches() {
  for(int i=0; i < AS_NUM_SWITCHES; i++) {
    pinMode(arcadeBtnsDin[i], INPUT_PULLUP);
    pinMode(arcadeBtnsLeds[i], OUTPUT);
  }
}

// ARCADE SWITCHES
void loopArcadeSwitches() {
  for(int i=0; i < AS_NUM_SWITCHES; i++) {
    int state = digitalRead(arcadeBtnsDin[i]);

    #ifdef DEBUG
      Serial.print("[ARCADESWITCHES] Switch on D");
      Serial.print(arcadeBtnsDin[i]);
      Serial.print(" = ");
      Serial.println(state);
    #endif

    digitalWrite(arcadeBtnsLeds[i], state);
  }
}

// PRESSURE GAUGE
void handleInterruptPressure() {
  int pressureButtonState = digitalRead(PRESSURE_BTN_PIN);
  pressureButtonState = pressureButtonState ? LOW : HIGH;

  if (pressureButtonState == HIGH) {
    pressurePercent+=10;
  } else {
    pressurePercent-=10;
  }

  if (pressurePercent > 100) {
    pressurePercent = 100;
  }

  if (pressurePercent < 0) {
    pressurePercent = 0;
  }
}

// PRESSURE GAUGE
void loopPressureGauge() {
  int pressurePercentCopy;

  noInterrupts();
  pressurePercentCopy = pressurePercent;
  interrupts();

  int pressureToLed = pressurePercentCopy / ( 100 / PRESSURE_NUM_LEDS); 
  long unsigned int pressureColor = pressureColors[pressureToLed];

  #ifdef DEBUG
    Serial.print("[PRESSUREGAUGE] pressureToLed = ");  
    Serial.print(pressureToLed);
    Serial.print("; pressurePercent = ");
    Serial.println(pressurePercentCopy);
  #endif

  for(int i = 0; i < pressureToLed; i++) {
    pressureLeds[i] = pressureColor;
  }

  for(int i = pressureToLed; i < PRESSURE_NUM_LEDS; i++) {
    pressureLeds[i] = CRGB::Black;
  }
}

// PRESSURE GAUGE
void setupPressureGauge() {
  pinMode(PRESSURE_BTN_PIN, INPUT);
  pinMode(PRESSURE_LED_PIN, OUTPUT);

  FastLED.addLeds<PRESSURE_LED_TYPE, PRESSURE_LED_PIN>(pressureLeds, PRESSURE_NUM_LEDS);
 
  Timer1.initialize(PRESSURE_TIMER_MICRO);
  Timer1.attachInterrupt(handleInterruptPressure);
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

  for(int i=0; i < SWITCHBOARD_NUM_PINS; i++) {
    matrixLeds[90 + i] = CRGB::Black;
  }
  
  // Connection found?
  if(switchboardActiveOut > 0 || switchboardActiveIn > 0) {
    matrixLeds[90 + switchboardActiveIn] = CRGB::Green;
    matrixLeds[90 + switchboardActiveOut] = CRGB::Red;
  }
}

void setup()
{
  Serial.begin(9600);

  setupMatrix();
  setupPotis();
  setupArcadeSwitches();
  setupPressureGauge();
  setupSwitchboard();

  // Finalize FastLED setup
  FastLED.setBrightness(LED_BRIGHTNESS);
  FastLED.clear();
  FastLED.show();
}

void loop()
{
  loopMatrix();
  loopPotis();
  loopArcadeSwitches();
  loopPressureGauge();
  loopSwitchboard();

  FastLED.show();
  delay(150);
}
