#include "GameSwitchboard.h"
 
// Constructor
GameSwitchboard::GameSwitchboard() {
}

// Destructor
GameSwitchboard::~GameSwitchboard() {
}

GameState GameSwitchboard::setup(GameState gameState) {
  gameState.innerState = 0;
  return gameState;
}

GameState GameSwitchboard::step(GameState gameState) {
}
