#include <Arduino.h>

#ifndef GAMESTATE
#define GAMESTATE
 
struct statechange
{
    unsigned int state;
    unsigned long msec;
};

class State {
  private:
    unsigned int nextState;
    unsigned long nextStateInMillis;

  public:
    unsigned int currentState;

    State(unsigned int initalState);
    ~State();

    // change state in n msecs
    void changeToState(unsigned int nextState, const unsigned long millis);

    // change state now
    void changeToState(unsigned int nextState);

    // Update state (put in loop())
    void tick();
};

#endif
