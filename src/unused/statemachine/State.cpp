#include <Arduino.h>
#include "State.h"

#ifdef DEBUG
  #include <HardwareSerial.h>
  extern HardwareSerial Serial;
#endif

/*
 * Usage:
 * 
 * State state = State(0);
 *
 * unsigned int aCallback(statechange state) {
 *   // Returning 0 will use the value from the call to changeToState(int, int, func)
 *
 *   return 41;
 * }
 *
 * void loop() {
 *   state.tick();
 *
 *   if (state.state == 42) { ... }
 *
 *   state.changeToState(4711, 1000, aCallback);
 * }
 */

State::State(unsigned int initialState)
{
  currentState = initialState;
  statechangesIndex = 0;
}

// Change the state to <state>
void State::changeToState(unsigned int state)
{
  changeToState(state, 0);
}

// Change the state to <state> in <nextInMillis> milliseconds
void State::changeToState(unsigned int state, const unsigned long nextInMillis)
{
  changeToState(state, nextInMillis, this->dummyCbFunc);
}

// Change the state to <state> or the return value od <cbFunc> in <nextInMillis> milliseconds
void State::changeToState(const unsigned int state, const unsigned long nextInMillis, unsigned int (*cbFunc)(const statechange state))
{
  unsigned long nextStateInMillis = millis() + nextInMillis;
  struct statechange nextState = { state, nextStateInMillis, cbFunc };
  statechanges[statechangesIndex] = nextState;

  // Sort by msec asc
  for (unsigned int i = 0; i < statechangesIndex; i++)
  {
    if (statechanges[i].msec > statechanges[i + 1].msec)
    {
      statechange temp = statechanges[i];
      statechanges[i] = statechanges[i + 1];
      statechanges[i + 1] = temp;
    }
  }

  statechangesIndex++;
}

// This needs to be called at the bottom of the Arduino standard "void loop()"
void State::tick()
{
  if (statechangesIndex > 0 && statechanges[0].msec <= millis()) {

    // Call function callback on transition
    unsigned int newState = statechanges[0].cbFunc(statechanges[0]);

    // If the callback returned a state > 0 use that as the new current state
    if (newState > 0) {
      currentState = newState;
    } else {
      // .. otherwise use the pre-programmed next state from the call to changeToState(int state, int, *func)
      currentState = statechanges[0].state;
    }

    // remove first entry on state stack
    for(unsigned int i = 1; i <= statechangesIndex; i++) {
      statechanges[i-1] = statechanges[i];
    }

    #ifdef DEBUG
      Serial.print("STATE INDEX = ");
      Serial.print(statechangesIndex);
      Serial.print(" currentState = ");
      Serial.println(currentState);
    #endif

    statechangesIndex--;
  }
}

unsigned int State::dummyCbFunc(const statechange state) {
  return 0;
}

