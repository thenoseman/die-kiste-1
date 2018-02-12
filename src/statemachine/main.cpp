#include <Arduino.h>
#include <HardwareSerial.h>
#include "State.h"

extern HardwareSerial Serial;

// Global game state
State state = State(0);

void setup()
{
  Serial.begin(9600);
  randomSeed(analogRead(3));
  state.changeToState(1, 10000);
  state.changeToState(2, 15000);
  state.changeToState(3, 20000);
}

void loop()
{
  state.tick();
  Serial.println(state.currentState);
}
