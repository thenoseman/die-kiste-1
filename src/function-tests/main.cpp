#include <Arduino.h>
#include <FastLED.h>
#include <HardwareSerial.h>

extern HardwareSerial Serial;

// Matrix display
#define MATRIX_PIN 2
#define MATRIX_NUM_LEDS 100
#define MATRIX_LED_TYPE NEOPIXEL
#define MATRIX_BRIGHTNESS 5
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

// *** MATRIX ***
void setupMatrix() {
  FastLED.addLeds<MATRIX_LED_TYPE, MATRIX_PIN>(matrixLeds, MATRIX_NUM_LEDS);
  FastLED.setBrightness(MATRIX_BRIGHTNESS);
  FastLED.clear();
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

  FastLED.show();
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

void setup()
{
  Serial.begin(9600);

  setupMatrix();
  setupPotis();
  setupArcadeSwitches();
}

void loop()
{
  loopMatrix();
  loopPotis();
  loopArcadeSwitches();
  delay(50);
}



