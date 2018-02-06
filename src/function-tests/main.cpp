#include <Arduino.h>
#include <FastLED.h>
#include <HardwareSerial.h>
#include <TimerOne.h>

extern HardwareSerial Serial;

// Common
#define LED_BRIGHTNESS 5

// Matrix display
#define MATRIX_PIN 2
#define MATRIX_NUM_LEDS 100
#define MATRIX_LED_TYPE NEOPIXEL
CRGB matrixLeds[MATRIX_NUM_LEDS];

// Potentimeters
#define POTIS_NUM_POTIS 4
const int potiPins[] = {0,1,2,3};
int potiActiveValues[] = {0,0,0,0};
int potiActiveIndex = 0;

// Arcade Switches
#define AS_NUM_SWITCHES 3
const int arcadeBtnsDin[] = {9, 11, 13};
const int arcadeBtnsLeds[] = {8, 10, 12};

// Pressure Gauge
#define PRESSURE_BTN_PIN 3
#define PRESSURE_LED_PIN 4
#define PRESSURE_NUM_LEDS 5
#define PRESSURE_LED_TYPE NEOPIXEL
#define PRESSURE_TIMER_MILLIS 100000 // 100 millisecs
CRGB pressureLeds[PRESSURE_NUM_LEDS];
volatile int pressurePercent = 0;
// These colors are used indicating the "danger" level, starting at 0 (no led lite) to 5 (all lite)
const long unsigned int pressureColors[] = {CRGB::Black, CRGB::LawnGreen, CRGB::Green, CRGB::Yellow, CRGB::Orange, CRGB::Red};

// Switchboard
#define SWITCHBOARD_NUM_PINS 6
const int switchboardPins[] = { 5, 6, 7, 8, 9, 10 };
int switchboardActiveOut = 0;
int switchboardActiveIn = 0;

// *** MATRIX ***
void setupMatrix() {
  FastLED.addLeds<MATRIX_LED_TYPE, MATRIX_PIN>(matrixLeds, MATRIX_NUM_LEDS);
}

// *** MATRIX ***
void loopMatrix() {

  // Loop setting potis:
  for (unsigned int potiIndex=0; potiIndex < 4; potiIndex++) {
    int mStart = potiIndex * 10;
    int mEnd = mStart + potiActiveValues[potiIndex] + 1;

    for (int i = mStart; i < mEnd; i++) {
      matrixLeds[i] = CRGB::Red;
    }

    for (int i = mEnd; i < (mStart + 10); i++) {
      matrixLeds[i] = CRGB::Yellow;
    }
  }

  // loop setting switchboard
}

// *** POTENTIOMETER ***
void setupPotis() {
}

// *** POTENTIOMETER ***
void loopPotis() {

  for (int potiIndex=0; potiIndex < POTIS_NUM_POTIS; potiIndex++) {
    int potiValue = map(analogRead(potiPins[potiIndex]), 0, 1023, 0, 9);

    // If one change is found, skill polling of other potentiometers
    // TODO: remove bouncing 0->1->0 effect
    if (potiValue != potiActiveValues[potiIndex]) {
      #ifdef DEBUG
        Serial.print("Potentiometer #");
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
    pinMode(arcadeBtnsDin[i], INPUT);
    pinMode(arcadeBtnsLeds[i], OUTPUT);
  }
}

// ARCADE SWITCHES
void loopArcadeSwitches() {
  for(int i=0; i < AS_NUM_SWITCHES; i++) {
    int state = digitalRead(arcadeBtnsDin[i]);

    #ifdef DEBUG
      Serial.print("AS Switch on D");
      Serial.print(arcadeBtnsDin[i]);
      Serial.print(" = ");
      Serial.print(state);
    #endif

    #ifdef DEBUG
      Serial.print(" -> setting to ");
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

  #ifdef DEBUG
    Serial.print("Pressure Gauge Btn = ");
    Serial.print(pressureButtonState);
    Serial.print("; pressurePercent = ");
    Serial.println(pressurePercent);
  #endif
}

// PRESSURE GAUGE
void loopPressureGauge() {
  int pressurePercentCopy;
  Serial.println("loopPressureGauge");

  noInterrupts();
  pressurePercentCopy = pressurePercent;
  interrupts();

  int pressureToLed = pressurePercentCopy / ( 100 / PRESSURE_NUM_LEDS); 
  long unsigned int pressureColor = pressureColors[pressureToLed];

  #ifdef DEBUG
    Serial.print("loopPressureGauge(): pressureToLed = ");  
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
 
  Timer1.initialize(PRESSURE_TIMER_MILLIS);
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

      #ifdef DEBUG
        Serial.print("[SWITCHBOARD] read input pin #");
        Serial.print(inputPinIndex);
        Serial.print(" (D");
        Serial.print(switchboardPins[inputPinIndex]);
        Serial.print(") = ");
        Serial.print(inputPinReadout);
        Serial.print(" Tried output #");
        Serial.print(outputPinIndex);
        Serial.print(" (D");
        Serial.print(switchboardPins[outputPinIndex]);
        Serial.print(") and input #");
        Serial.print(inputPinIndex);
        Serial.print(" (D");
        Serial.print(switchboardPins[inputPinIndex]);
        Serial.println(")");
      #endif

      if (inputPinReadout == LOW) {
        switchboardActiveIn = inputPinIndex;
        switchboardActiveOut = outputPinIndex;
        #ifdef DEBUG
          Serial.print("[SWITCHBOARD] connection from IN ");
          Serial.print(switchboardActiveIn);
          Serial.print(" (D");
          Serial.print(switchboardPins[switchboardActiveIn]);
          Serial.print(") to OUT ");
          Serial.print(switchboardActiveOut);
          Serial.print(" (D");
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
  if(switchboardActiveOut > 0 && switchboardActiveIn > 0) {
    matrixLeds[90 + switchboardActiveIn] = CRGB::Green;
    matrixLeds[90 + switchboardActiveOut] = CRGB::Red;
  }

  delay(2000);
}

void setup()
{
  Serial.begin(9600);

  setupMatrix();
  //setupPotis();
  //setupArcadeSwitches();
  //setupPressureGauge();
  setupSwitchboard();

  // Finalize FastLED setup
  FastLED.setBrightness(LED_BRIGHTNESS);
  FastLED.clear();
  FastLED.show();
}

void loop()
{
  loopMatrix();
  //loopPotis();
  //loopArcadeSwitches();
  //loopPressureGauge();
  loopSwitchboard();

  FastLED.show();
}



