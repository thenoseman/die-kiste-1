#include "State.h"

struct statechange
{
    unsigned int state;
    unsigned long msec;
};

State::State(unsigned int initialState)
{
  struct statechange statechanges[10];
}


State::~State()
{
}


void State::changeToState(unsigned int state, const unsigned long nextInMillis)
{
  nextState = state;
  nextStateInMillis = millis() + nextInMillis;
  struct statechange nextState = { state, nextStateInMillis};
  
}


void State::changeToState(unsigned int state)
{
  nextState = state;
}


void State::tick()
{
  if (nextStateInMillis > 0 && millis() >= nextStateInMillis) {
    currentState = nextState;
    nextState = 0;
    nextStateInMillis = 0;
  }
}


