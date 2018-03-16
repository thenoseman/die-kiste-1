#include <Arduino.h>
#include <FastLED.h>

//                                  +-----+
//                     +------------| USB |-------------+
//                     |            +-----+             |
//                     | [ ]D13/SCK         MISO/D12[ ] |
//                     | [ ]3.3V            MOSI/D11[ ]~|
//                     | [ ]V.ref     ___     SS/D10[ ]~|
// MATRIX_LED_PIN   <- | [ ]A0       / N \        D9[ ]~|
//                     | [ ]A1      /  A  \       D8[ ] |
//                     | [ ]A2      \  N  /       D7[ ] |
//                     | [ ]A3       \_0_/        D6[ ]~|
//                     | [ ]A4/SDA                D5[ ]~|
//                     | [ ]A5/SCL                D4[ ] |
//                     | [ ]A6 (noDIn)       INT1/D3[ ]~|
//                     | [ ]A7 (noDIn)       INT0/D2[ ] |
//                     | [ ]5V                   GND[ ] |
//                     | [ ]RST                  RST[ ] |
//                     | [ ]GND   5V MOSI GND D0/RX1[ ] |
//                     | [ ]Vin   [ ] [ ] [ ] D1/TX1[ ] |
//                     |          [ ] [ ] [ ]           |
//                     |          MISO SCK RST          |
//                     | NANO-V3                        |
//                     +--------------------------------+

const byte alphaElementSize = 13;
const byte matrixDinPin = A0;
const byte numLeds = 100;
const byte matrixLedBrightness = 20;
CRGB leds[numLeds];

void setup()
{
  FastLED.addLeds<NEOPIXEL, matrixDinPin>(leds, numLeds);
  FastLED.setBrightness(matrixLedBrightness);
  FastLED.clear();
}

void loop()
{
  fill_solid(leds, numLeds, CRGB::Green);
  FastLED.show();
}
