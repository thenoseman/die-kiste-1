#include <Arduino.h>
#include "State.h"

extern HardwareSerial Serial;

State::State(unsigned int initialState)
{
  currentState = initialState;
  statechangesIndex = 0;
}


State::~State()
{
}


void State::changeToState(unsigned int state, const unsigned long nextInMillis)
{
  changeToState(state, nextInMillis, this->dummyCbFunc);
}


void State::changeToState(unsigned int state)
{
  changeToState(state, 0);
}

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

