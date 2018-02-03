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
int potiPins[] = {0,1,2,3};
int potiActiveValues[] = {0,0,0,0};
int potiActiveIndex = 0;

// Arcade Switches
#define AS_NUM_SWITCHES 3
int arcadeBtnsDin[] = {9, 11, 13};
int arcadeBtnsLeds[] = {8, 10, 12};

// Pressure Gauge
#define PRESSURE_BTN_PIN 2
#define PRESSURE_LED_PIN 3
#define PRESSURE_NUM_LEDS 5
#define PRESSURE_LED_TYPE NEOPIXEL
#define PRESSURE_TIMER_MILLIS 100000 // 100 millisecs
CRGB pressureLeds[PRESSURE_NUM_LEDS];
volatile int pressurePercent = 0;
// These colrs are used indicating the "danger" level, starting at 0 (no led lite) to 5 (all lite)
long unsigned int pressureColors[] = {CRGB::Black, CRGB::LawnGreen, CRGB::Green, CRGB::Yellow, CRGB::Orange, CRGB::Red};

// *** MATRIX ***
void setupMatrix() {
  FastLED.addLeds<MATRIX_LED_TYPE, MATRIX_PIN>(matrixLeds, MATRIX_NUM_LEDS);
}

// *** MATRIX ***
void loopMatrix() {
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
    pinMode(arcadeBtnsDin[i], INPUT_PULLUP);
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
  pinMode(PRESSURE_BTN_PIN, INPUT_PULLUP);
  pinMode(PRESSURE_LED_PIN, OUTPUT);

  FastLED.addLeds<PRESSURE_LED_TYPE, PRESSURE_LED_PIN>(pressureLeds, PRESSURE_NUM_LEDS);
 
  Timer1.initialize(PRESSURE_TIMER_MILLIS);
  Timer1.attachInterrupt(handleInterruptPressure);
}

void setup()
{
  Serial.begin(9600);

  setupMatrix();
  setupPotis();
  setupArcadeSwitches();
  setupPressureGauge();

  // Finalize FastLED  setup
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

  FastLED.show();
}



