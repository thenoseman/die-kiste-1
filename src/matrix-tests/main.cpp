#include <Arduino.h>
#include <HardwareSerial.h>
#include "Matrix.h"

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
                   
Matrix matrix = Matrix();

// *** MATRIX ***
void setupMatrix() {
  matrix.show('a');
}

// *** MATRIX ***
void loopMatrix() {
  matrix.update();
}

void setup()
{
  Serial.begin(9600);

  setupMatrix();
}

void loop()
{
  loopMatrix();
}
