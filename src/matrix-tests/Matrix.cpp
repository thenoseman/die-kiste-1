#include "Matrix.h"
#include <FastLED.h>

Matrix::Matrix()
{
  FastLED.addLeds<NEOPIXEL, dinPin>(leds, numLeds);
  FastLED.setBrightness(brightness);
  FastLED.clear();
  changed = 0;
}

void Matrix::show(int alphaIndex, int startColumn, int startRow, CRGB color)
{
  changed = 1;
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
  changed = 1;

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

void Matrix::picture(int picture[], int startColumn, int startRow, CRGB color) {
  changed = 1;

  int length =  sizeof(&picture)/sizeof(*picture);
  for(int i=0; i < length; i++) {
    leds[i + ( sizeXY * startColumn ) + startRow] = color;
  }
}

void Matrix::update() {
  if (changed == 1) {
    FastLED.show();
    changed = 0;
  }
}


