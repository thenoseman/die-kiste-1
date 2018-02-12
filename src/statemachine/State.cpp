#include "State.h"

extern HardwareSerial Serial;

State::State(unsigned int initialState)
{
  currentState = initialState;
  statechangesIndex= 0;
}


State::~State()
{
}


void State::changeToState(unsigned int state, const unsigned long nextInMillis)
{
  unsigned long nextStateInMillis = millis() + nextInMillis;
  struct statechange nextState = { state, nextStateInMillis };
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


void State::changeToState(unsigned int state)
{
  changeToState(state, 0);
}


void State::tick()
{
  if (statechanges[0].msec <= millis()) {
    currentState = statechanges[0].state;

    // move index 1-n up
    for(unsigned int i = 1; i <= statechangesIndex; i++) {
      statechanges[i-1] = statechanges[i];
    }
    statechangesIndex--;
  }
}


