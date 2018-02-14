#ifndef GAMESTATE
#define GAMESTATE

#include <Arduino.h>
 
struct statechange
{
    unsigned int state;
    unsigned long msec;
    unsigned int (*cbFunc)(statechange state);
};

class State {
  private:
    struct statechange statechanges[10];
    unsigned int statechangesIndex = 0;
    static unsigned int dummyCbFunc(statechange state);

  public:
    unsigned int currentState;

    State(unsigned int initalState);

    // change state NOW
    void changeToState(const unsigned int nextState);

    // change state in X milliseconds
    void changeToState(const unsigned int nextState, const unsigned long millis);

    // change state in X milliseconds. Will call the cbFunc and use its return value as new state if it is > 0
    void changeToState(const unsigned int nextState, const unsigned long millis, unsigned int (*cbFunc)(const statechange state));

    // Update state (put in loop())
    void tick();
};

#endif
