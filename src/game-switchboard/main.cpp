#include <Arduino.h>
#include <FastLED.h>
#include <HardwareSerial.h>
#include <TimerOne.h>
#include <GameState.h>
#include <GameSwitchboard.h>

extern HardwareSerial Serial;

//                                  +-----+
//                     +------------| USB |-------------+
//                     |            +-----+             |
//                     | [ ]D13/SCK         MISO/D12[ ] |
//                     | [ ]3.3V            MOSI/D11[ ]~|
//                     | [ ]V.ref     ___     SS/D10[ ]~|
//                     | [ ]A0       / N \        D9[ ]~|
//                     | [ ]A1      /  A  \       D8[ ] |
// MATRIX_LED_PIN   <- | [ ]A2      \  N  /       D7[ ] | <- switchboardPins[5]
//                     | [ ]A3       \_0_/        D6[ ]~| <- switchboardPins[4]
//                     | [ ]A4/SDA                D5[ ]~| <- switchboardPins[3]
//                     | [ ]A5/SCL                D4[ ] | <- switchboardPins[2]
//                     | [ ]A6 (noDIn)       INT1/D3[ ]~| <- switchboardPins[1]
//                     | [ ]A7 (noDIn)       INT0/D2[ ] | <- switchboardPins[0]
//                     | [ ]5V                   GND[ ] |
//                     | [ ]RST                  RST[ ] |
//                     | [ ]GND   5V MOSI GND D0/RX1[ ] |
//                     | [ ]Vin   [ ] [ ] [ ] D1/TX1[ ] |
//                     |          [ ] [ ] [ ]           |
//                     |          MISO SCK RST          |
//                     | NANO-V3                        |
//                     +--------------------------------+
                   
// Common
#define LED_BRIGHTNESS 5

// Matrix display
const uint8_t MATRIX_LED_PIN = A2;
#define MATRIX_NUM_LEDS 100
#define MATRIX_DIM 10
#define MATRIX_LED_TYPE NEOPIXEL
CRGB matrixLeds[MATRIX_NUM_LEDS];

// Switchboard
#define SWITCHBOARD_NUM_PINS 6
const int switchboardPins[] = { 2, 3, 4, 5, 6, 7 };
int switchboardActiveOut = 0;
int switchboardActiveIn = 0;

#define GAME_NOGAME 0
#define GAME_SWITCHBOARD 1

// All games
const byte games[] = { GAME_SWITCHBOARD };

// Global game state
GameState gameState = { GAME_NOGAME, 0, 0 };
GameSwitchboard gameSwitchboard = GameSwitchboard();

// *** MATRIX ***
void setupMatrix() {
  FastLED.addLeds<MATRIX_LED_TYPE, MATRIX_LED_PIN>(matrixLeds, MATRIX_NUM_LEDS);
}

// *** MATRIX ***
void loopMatrix() {
}

// SWITCHBOARD
void setupGameSwitchboard() {
  // Reset all Pins as INPUTs
  // todo: move to class!
  for (int i=0; i < SWITCHBOARD_NUM_PINS; i++) {
    pinMode(switchboardPins[i], INPUT_PULLUP);
  }
  switchboardActiveOut = 0;
  switchboardActiveIn = 0;
}

GameState chooseRandomGame(GameState gameState) {
  gameState.activeGame = random(0, sizeof(games)/sizeof(*games));

  #ifdef DEBUG
    Serial.print("[STATE] active game = ");
    Serial.println(gameState.activeGame);
  #endif

  return gameState;
}

void setup()
{
  Serial.begin(9600);
  randomSeed(analogRead(3));

  gameState = chooseRandomGame(gameState);

  setupMatrix();
  gameState = gameSwitchboard.setup(gameState);

  // Finalize FastLED setup
  FastLED.setBrightness(LED_BRIGHTNESS);
  FastLED.clear();
  FastLED.show();
}

void loopGame() {
  gameState.millis = millis();
  gameSwitchboard.step(gameState);
}

void loop()
{
  loopGame();
  loopMatrix();

  FastLED.show();
  delay(100);
}
