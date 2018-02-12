#include <Arduino.h>
#include <HardwareSerial.h>
#include "State.h"

extern HardwareSerial Serial;

// Global game state
State state = State(0);

unsigned int blubb(statechange state) {
  Serial.println("BLUBB");
  return 1;
}

void setup()
{
  Serial.begin(9600);
  randomSeed(analogRead(3));
  
  state.changeToState(1, 3000, blubb);
  state.changeToState(2, 5000);
}

void loop()
{
  state.tick();
  if( millis() % 500 == 0) {
    Serial.println(state.currentState);
  }
}
