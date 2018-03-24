#include <Arduino.h>
#include <HardwareSerial.h>
extern HardwareSerial Serial;
//                                  +-----+
//                      +------------| USB |-------------+
//                      |            +-----+             |
//                      | [ ]D13/SCK/noDin   MISO/D12[ ] | 
//                      | [ ]3.3V            MOSI/D11[ ]~| 
//                      | [ ]V.ref     ___     SS/D10[ ]~| 
//                      | [ ]A0       / N \        D9[ ]~| 
//                      | [ ]A1      /  A  \       D8[ ] | 
//                      | [ ]A2      \  N  /       D7[ ] | 
//                      | [ ]A3       \_0_/        D6[ ]~| 
// LT  potiPins[0]   -> | [ ]A4/SDA                D5[ ]~| 
// RT  potiPins[1]   -> | [ ]A5/SCL                D4[ ] | 
// LB  potiPins[2]   -> | [ ]A6 (noDIn)       INT1/D3[ ]~| 
// RB  potiPins[3]   -> | [ ]A7 (noDIn)       INT0/D2[ ] | 
//                      | [ ]5V                   GND[ ] |
//                      | [ ]RST                  RST[ ] |
//                      | [ ]GND   5V MOSI GND D0/RX1[ ] |
//                      | [ ]Vin   [ ] [ ] [ ] D1/TX1[ ] |
//                      |          [ ] [ ] [ ]           |
//                      |          MISO SCK RST          |
//                      | NANO-V3                        |
//                      +--------------------------------+
//
// Poti Cables:
// ------------
// Poti 1: Yellow
// Poti 2: Blue
// Poti 3: Orange
// Poti 4: Green

void setup()
{
  Serial.begin(9600);
  Serial.println("SETUP");

  pinMode(A4, INPUT_PULLUP);
  pinMode(A5, INPUT_PULLUP);
  pinMode(A6, INPUT_PULLUP);
  pinMode(A7, INPUT_PULLUP);
}

void printnr(String pin, unsigned int reading) {
  Serial.print(pin);
  Serial.print("=");

  if(reading < 1000) {
    Serial.print("0");
  }

  if(reading < 100) {
    Serial.print("0");
  }

  if(reading < 10) {
    Serial.print("0");
  }
  
  Serial.print(reading);
  Serial.print("; ");
}

void loop()
{
  printnr("A4", analogRead(4));
  printnr("A5", analogRead(5));
  printnr("A6", analogRead(6));
  printnr("A7", analogRead(7));

  Serial.println("");
  delay(250);
}
