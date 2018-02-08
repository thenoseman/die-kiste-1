#include <Arduino.h>

#ifndef GAMESTATE
#define GAMESTATE

typedef struct GameState GameState;

struct GameState {
  byte activeGame;
  byte innerState;
  unsigned long millis;
};

#endif
