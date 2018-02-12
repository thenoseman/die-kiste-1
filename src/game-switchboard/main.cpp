#include <Arduino.h>
#include <HardwareSerial.h>
#include "State.h"

extern HardwareSerial Serial;

#define GAME_NOGAME 0
#define GAME_SWITCHBOARD 1

// All games
const byte games[] = { GAME_SWITCHBOARD };

// Global game state
State state = State(0);

void setup()
{
  Serial.begin(9600);
  randomSeed(analogRead(3));
  state.changeToState(1, 3000);
}

void loop()
{
  state.tick();
  Serial.print("Current state = ");
  Serial.println(state.currentState);
}
