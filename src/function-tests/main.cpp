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
int potiPins[] = {0,1,2,3};
int potiOldValues[] = {0,0,0,0};

void setupMatrix() {
  FastLED.addLeds<MATRIX_LED_TYPE, MATRIX_PIN>(matrixLeds, MATRIX_NUM_LEDS);
  FastLED.setBrightness(MATRIX_BRIGHTNESS);
}

void loopMatrix() {
  for(int i=0; i < MATRIX_NUM_LEDS; i++) {
    matrixLeds[i] = CRGB::White;
  }
  FastLED.show();
  delay(2000);
}

void setupPotis() {
}

void loopPotis() {
  #ifdef DEBUG
    for(int i=0; i < 4; i++) {
      // read value from poti and map to 0-9
      int potiValue = analogRead(i);
      Serial.print("Potentiometer");
      Serial.print(i);
      Serial.print(" = ");
      Serial.println(potiValue);
    }
    delay(3000);
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



