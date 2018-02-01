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
int fillToRow = 0;
long unsigned int fillWithColors[] = {
  CRGB::Red,
  CRGB::Yellow,
  CRGB::Blue,
  CRGB::White
};

// Potentimeters
int potiPins[] = {0,1,2,3};
int potiActiveValues[] = {0,0,0,0};
int potiActiveIndex = 0;


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
  #ifdef DEBUG
    for (unsigned int potiIndex=0; potiIndex < sizeof(potiPins); potiIndex++) {
      int potiValue = map(analogRead(potiPins[potiIndex]), 0, 1023, 0, 9);

      // If one change is found, skill polling of other potentiometers
      if (potiValue != potiActiveValues[potiIndex]) {
        Serial.print("Potentiometer #");
        Serial.print(potiIndex);
        Serial.print(" on A");
        Serial.print(potiPins[potiIndex]);
        Serial.print(" = ");
        Serial.println(potiValue);

        // This will be picked up by loopMatrix()
        potiActiveValues[potiIndex] = potiValue;
        potiActiveIndex = potiIndex;
      }
    }
  #endif
}

void setup()
{
  Serial.begin(9600);

  setupMatrix();
  setupPotis();
}

void loop()
{
  loopMatrix();
  loopPotis();
}



