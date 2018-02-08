#ifndef GAMESWITCHBOARD
#define GAMESWITCHBOARD
 
#include <Arduino.h>
#include "GameState.h"
 
class GameSwitchboard {
  private:
    byte someThing;

  public:
    GameSwitchboard();
    ~GameSwitchboard();

    GameState setup(const GameState gameState);
    GameState step(const GameState gameState);
};
 
#endif
