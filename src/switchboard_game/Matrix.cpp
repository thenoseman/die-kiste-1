#include "Matrix.h"

#ifndef __INC_FASTSPI_LED2_H
#include <FastLED.h>
#endif

Matrix::Matrix()
{
  FastLED.addLeds<NEOPIXEL, dinPin>(leds, numLeds);
  FastLED.setBrightness(brightness);
  FastLED.clear();
}

void Matrix::show(int alphaIndex, int startColumn, int startRow, CRGB color)
{
  for(int i=0; i < alphaElementSize ; i++) {
    int currentLedPos = alpha[alphaIndex][i];

    if(currentLedPos == -1) {
      break;
    }

    leds[currentLedPos + ( sizeXY * startColumn ) + startRow] = color;
  }
}

void Matrix::show(char character, int startColumn, int startRow, CRGB color) {
  int charInt = (int)character - 48;

  // 0-9
  if(charInt < 10) {
    show(charInt, startColumn, startRow, color);
  }

  // a-z
  if(charInt >= 49 && charInt <= 74) {
    show((charInt - 39), startColumn, startRow, color);
  }
}

void Matrix::show(char character) {
  show(character, 0, 0, CRGB::White);
}

void Matrix::showSpecial(int special[], int startColumn, int startRow, CRGB color) {
  int length =  sizeof(&special)/sizeof(*special);
  for(int i=0; i < length; i++) {
    leds[i + ( sizeXY * startColumn ) + startRow] = color;
  }
}

void Matrix::update() {
  FastLED.show();
}


