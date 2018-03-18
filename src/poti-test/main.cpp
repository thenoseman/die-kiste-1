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
}

void loop()
{
  Serial.print("A4: ");
  Serial.print(analogRead(4));
  Serial.print("; A5: ");
  Serial.print(analogRead(5));
  Serial.print("; A6: ");
  Serial.print(analogRead(6));
  Serial.print("; A7: ");
  Serial.print(analogRead(7));
  Serial.println("");
  
  delay(250);
}
