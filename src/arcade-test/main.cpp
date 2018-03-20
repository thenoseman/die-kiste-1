#include <Arduino.h>
#include <HardwareSerial.h>
extern HardwareSerial Serial;


//                                  +-----+
//                      +------------| USB |-------------+
//                      |            +-----+             |
// arcadeBtnsLeds[2] <- | [ ]D13/SCK/noDin   MISO/D12[ ] | -> arcadeBtnsLeds[1]
//                      | [ ]3.3V            MOSI/D11[ ]~| -> arcadeBtnsLeds[0]
//                      | [ ]V.ref     ___     SS/D10[ ]~| <- arcadeBtnsDin[2]
//                      | [ ]A0       / N \        D9[ ]~| <- arcadeBtnsDin[1]
//                      | [ ]A1      /  A  \       D8[ ] | <- arcadeBtnsDin[0]
//                      | [ ]A2      \  N  /       D7[ ] |                                                          
//                      | [ ]A3       \_0_/        D6[ ]~|                            
//                      | [ ]A4/SDA                D5[ ]~|                            
//                      | [ ]A5/SCL                D4[ ] |                           
//                      | [ ]A6 (noDIn)       INT1/D3[ ]~|                                             
//                      | [ ]A7 (noDIn)       INT0/D2[ ] |                                 
//                      | [ ]5V                   GND[ ] |
//                      | [ ]RST                  RST[ ] |
//                      | [ ]GND   5V MOSI GND D0/RX1[ ] |
//                      | [ ]Vin   [ ] [ ] [ ] D1/TX1[ ] |
//                      |          [ ] [ ] [ ]           |
//                      |          MISO SCK RST          |
//                      | NANO-V3                        |
//                      +--------------------------------+
void setup()
{
  Serial.begin(9600);
  Serial.println("SETUP");

  pinMode(11, OUTPUT);
  pinMode(12, OUTPUT);
  pinMode(13, OUTPUT);

  pinMode(8, INPUT_PULLUP);
  pinMode(9, INPUT_PULLUP);
  pinMode(10, INPUT_PULLUP);
}

void loop()
{
  for (uint8_t i=0; i < 3; i++) {

    Serial.print("D");
    Serial.print(i + 8);
    Serial.print(" = ");
    Serial.println(digitalRead(i + 8));

    if (digitalRead(i + 8) == LOW) {
      digitalWrite(i + 11, HIGH);
    } else {
      digitalWrite(i + 11, LOW);
    }
  }

  delay(250);
}
