#include <Arduino.h>
#include <HardwareSerial.h>
#include <FastLED.h>

extern HardwareSerial Serial;

//                                  +-----+
//                     +------------| USB |-------------+
//                     |            +-----+             |
// arcadeBtnsDin[2] -> | [ ]D13/SCK         MISO/D12[ ] | -> arcadeBtnsLeds[2]
//                     | [ ]3.3V            MOSI/D11[ ]~| <- arcadeBtnsDin[1]
//                     | [ ]V.ref     ___     SS/D10[ ]~| -> arcadeBtnsLeds[1]
// PRESSURE_LED_PIN <- | [ ]A0       / N \        D9[ ]~| <- arcadeBtnsDin[0]
// PRESSURE_BTN_PIN -> | [ ]A1      /  A  \       D8[ ] | -> arcadeBtnsLeds[0]
// MATRIX_LED_PIN   <- | [ ]A2      \  N  /       D7[ ] | <- switchboardPins[5] => 64
//                     | [ ]A3       \_0_/        D6[ ]~| <- switchboardPins[4] => 32
//     potiPins[0]  -> | [ ]A4/SDA                D5[ ]~| <- switchboardPins[3] => 16
//     potiPins[1]  -> | [ ]A5/SCL                D4[ ] | <- switchboardPins[2] => 8
//     potiPins[2]  -> | [ ]A6 (noDIn)       INT1/D3[ ]~| <- switchboardPins[1] => 4
//     potiPins[3]  -> | [ ]A7 (noDIn)       INT0/D2[ ] | <- switchboardPins[0] => 2
//                     | [ ]5V                   GND[ ] |
//                     | [ ]RST                  RST[ ] |
//                     | [ ]GND   5V MOSI GND D0/RX1[ ] |
//                     | [ ]Vin   [ ] [ ] [ ] D1/TX1[ ] |
//                     |          [ ] [ ] [ ]           |
//                     |          MISO SCK RST          |
//                     | NANO-V3                        |
//                     +--------------------------------+

const int alphaElementSize = 14;
const int matrixDinPin = A2;
const int numLeds = 100;
const int matrixLedBrightness = 5;
int ledsModified = 0;
CRGB leds[numLeds];

// Global game state
int activeGame = 0;
long numberOfGames = 1;
unsigned long now;

const int gameSwitchboardPins[] = { 2, 3, 4, 5, 6, 7 };
const int gameSwitchboardPinValues[] = { 2, 4, 8, 16, 32, 64 };
const int gameSwitchboardPinNum = 6;
int gameSwitchboardPinActive1 = 0;
int gameSwitchboardPinActive2 = 0;
typedef struct SwitchBoard {
  int activePin1;
  int activePin2;
  int number;
} Switchboard;
Switchboard switchboard = { .activePin1 = 0, .activePin2 = 0 };

typedef struct State {
  unsigned int current;
  unsigned int next;
  unsigned long nextStateAtMsec;
} State;
State state = { .current = 0, .next = 0, .nextStateAtMsec = 0};

void changeStateTo(const unsigned int nextState, const unsigned long nextStateInMsec) { /*{{{*/
  if (state.next != nextState) {
    state.nextStateAtMsec = millis() + nextStateInMsec;
    state.next = nextState;

    #ifdef DEBUG
      Serial.print("changeStateTo: next = ");  
      Serial.print(state.next);
      Serial.print(", at msec = ");
      Serial.println(state.nextStateAtMsec);
    #endif
  }
} /*}}} */

void updateState() { /*{{{*/
  if (state.nextStateAtMsec > 0 && state.nextStateAtMsec <= millis()) {
    state.current = state.next;
    state.nextStateAtMsec = 0;
    state.next = 0;
    #ifdef DEBUG
      Serial.print("updateState: state.current = ");
      Serial.println(state.current);
    #endif
  }
} /*}}} */

/* alpha {{{*/
const unsigned int alpha[10][alphaElementSize] = {
  {10,11,12,13,20,24,31,32,33,34,255}, // 0
  {13,20,21,22,23,24,255}, // 1
  {10,11,14,20,22,24,30,33,255}, // 2
  {10,14,20,22,24,31,33,255}, // 3
  {12,13,14,22,30,31,32,33,34,255}, // 4
  {10,12,13,14,20,22,24,31,34,255}, // 5
  {10,11,12,13,20,22,24,30,31,32,34,255}, // 6
  {10,11,14,22,24,33,34,255}, // 7
  {10,11,12,13,14,20,22,24,30,31,32,33,34,255}, // 8
  {10,12,13,14,20,22,24,31,32,33,34,255}, // 9
  //{10,11,12,13,22,24,30,31,32,33,255}, // A
  //{10,11,12,13,14,20,22,24,31,33,255}, // B
  //{11,12,13,20,24,30,34,255}, // C
  //{10,11,12,13,14,20,24,31,32,33,255}, // D
  //{10,11,12,13,14,20,22,24,30,32,34,255}, // E
  //{10,11,12,13,14,22,24,32,34,255}, // F
  //{11,12,13,20,22,24,30,31,32,34,255}, // G
  //{10,11,12,13,14,22,30,31,32,33,34,255}, // H
  //{10,14,20,21,22,23,24,30,34,255}, // I
  //{11,20,31,32,33,34,255}, // J
  //{10,11,12,13,14,22,30,31,33,34,255}, // K
  //{10,11,12,13,14,20,30,255}, // L
  //{10,11,12,13,14,22,23,30,31,32,33,34,255}, // M
  //{10,11,12,13,14,21,22,23,30,31,32,33,34,255}, // N
  //{11,12,13,20,24,31,32,33,255}, // O
  //{10,11,12,13,14,22,24,33,255}, // P
  //{11,12,13,20,21,24,30,31,32,33,255}, // Q
  //{10,11,12,13,14,21,22,24,30,32,33,255}, // R
  //{10,13,20,22,24,31,34,255}, // S
  //{14,20,21,22,23,24,34,255}, // T
  //{11,12,13,14,20,30,31,32,33,34,255}, // U
  //{12,13,14,20,21,32,33,34,255}, // V
  //{10,11,12,13,14,21,22,30,31,32,33,34,255}, // W
  //{10,11,13,14,22,30,31,33,34,255}, // X
  //{13,14,20,21,22,33,34,255}, // Y
  //{10,11,14,20,22,24,30,33,34,255}  // Z
};
/*}}}*/

/* pictures {{{*/
const int matrixPicBig3[35] = {21,22,27,28,31,32,37,38,41,42,44,45,47,48,51,52,54,55,57,58,61,62,63,64,65,66,67,68,72,73,74,75,76,77,-1};
const int matrixPicBig2[35] = {21,22,27,28,31,32,33,37,38,41,42,43,44,47,48,51,52,53,54,55,57,58,61,62,64,65,66,67,68,71,72,75,76,77,-1};
const int matrixPicBig1[25] = {25,26,35,36,37,46,47,48,51,52,53,54,55,56,57,58,61,62,63,64,65,66,67,68,-1};
const int matrixPicRunner[25] = {10,11,14,21,25,32,33,35,42,43,44,45,50,51,53,54,55,56,57,60,63,66,67,74,-1};
const int matrixPicSmileyPositive[75] = {2,3,4,5,6,7,11,12,13,14,15,16,17,18,20,21,22,24,27,28,29,30,31,33,34,37,38,39,40,41,43,44,45,46,47,48,49,50,51,53,54,55,56,57,58,59,60,61,63,64,67,68,69,70,71,72,74,77,78,79,81,82,83,84,85,86,87,88,92,93,94,95,96,97,-1};
const int matrixPicSmileyNegative[77] = {2,3,4,5,6,7,11,12,13,14,15,16,17,18,20,21,23,24,27,28,29,30,31,32,34,36,37,38,39,40,41,42,44,45,46,47,48,49,50,51,52,54,55,56,57,58,59,60,61,62,64,66,67,68,69,70,71,73,74,77,78,79,81,82,83,84,85,86,87,88,92,93,94,95,96,97,-1};
const int matrixPicSkull[44] = {14,15,16,17,18,22,23,24,25,26,28,29,33,34,35,36,38,39,42,43,44,46,47,48,49,53,54,55,56,58,59,62,63,64,65,66,68,69,74,75,76,77,78,-1};
const int matrixPicBox[38] = {1,2,3,4,5,6,11,16,17,21,26,28,31,36,39,41,46,49,51,52,53,54,55,56,59,62,67,69,73,78,79,84,85,86,87,88,89,-1};
/*}}}*/

void matrixSetByIndex(unsigned int alphaIndex, int startColumn, int startRow, CRGB color) /*{{{*/{
  ledsModified = 1;
  int currentLedPos = 0;
  
  for(int i = 0; i < alphaElementSize; i++) {
    currentLedPos = alpha[alphaIndex][i];

    if(currentLedPos == 255) {
      break;
    }

    leds[currentLedPos + ( 10 * startColumn ) + startRow] = color;
  }
} /*}}}*/

void matrixSetByArray(const int activeLeds[], int startColumn, int startRow, CRGB color) /*{{{*/{
  ledsModified = 1;

  for(int i=0; i < numLeds; i++) {
    if (activeLeds[i] == -1) {
      break;
    }

    leds[activeLeds[i] + ((numLeds/numLeds) * startColumn ) + startRow] = color;
  }
} /*}}}*/

void matrix_setup() /*{{{*/{
  FastLED.addLeds<NEOPIXEL, matrixDinPin>(leds, numLeds);
  FastLED.setBrightness(matrixLedBrightness);
} /*}}}*/

void game_setup() {/*{{{*/
  randomSeed(analogRead(4));
  changeStateTo(1, 1);
} /*}}}*/

void setup() /*{{{*/
{
  Serial.begin(9600);
  
  matrix_setup();
  game_setup();
}/*}}}*/

void game_intro_loop() { /*{{{*/
  #ifdef DEBUG
    Serial.print("game_intro_loop: state = ");
    Serial.println(state.current);
  #endif

  switch (state.current) {
    case 1:
      matrixSetByArray(matrixPicBig3, 0, 0, CRGB::Red);
      changeStateTo(2, 1000);
      break;
    case 2:
      matrixSetByArray(matrixPicBig2, 0, 0, CRGB::Orange);
      changeStateTo(3, 1000);
      break;
    case 3:
      matrixSetByArray(matrixPicBig1, 0, 0, CRGB::Yellow);
      changeStateTo(4, 1000);
      break;
    case 4:
      matrixSetByArray(matrixPicRunner, 1, 0, CRGB::Green);
      changeStateTo(10, 1000);
      break;
  }
} /*}}}*/

void game_choose() { /*{{{*/
  activeGame = random(1, numberOfGames + 1); 

  // Every game has 20 possible states
  changeStateTo(activeGame * 20, 1);

  #ifdef DEBUG
    Serial.print("game_choose: activeGame = ");
    Serial.print(activeGame);
    Serial.print(", nextState = ");
    Serial.println(state.next);
  #endif
} /*}}} */

void game_switchboard_reset() { /*{{{*/
  switchboard.activePin1 = random(0, gameSwitchboardPinNum);

  do {
    switchboard.activePin2 = random(0, gameSwitchboardPinNum);
  } while(switchboard.activePin1 == switchboard.activePin2);

  switchboard.number = gameSwitchboardPinValues[switchboard.activePin1] + gameSwitchboardPinValues[switchboard.activePin2];

  #ifdef DEBUG
    Serial.print("game_switchboard_reset: activePin1 = ");
    Serial.print(switchboard.activePin1);
    Serial.print(", activePin2 = ");
    Serial.print(switchboard.activePin2);
    Serial.print(", number = ");
    Serial.println(switchboard.number);
  #endif

  // Reset all Pins as INPUTs
  for (int i=0; i < gameSwitchboardPinNum; i++) {
    pinMode(gameSwitchboardPins[i], INPUT_PULLUP);
  }
  gameSwitchboardPinActive1 = 0;
  gameSwitchboardPinActive2 = 0;

  // Display target number
  matrixSetByIndex((switchboard.number/10), 1, 0, CRGB::Yellow);
  matrixSetByIndex((switchboard.number%10), 5, 0, CRGB::Yellow);

  // Start game loop
  changeStateTo(21, 1);
} /*}}} */

// STATE: 21+
void game_switchboard_loop() { /*{{{*/
  // Reset
  int inputPinReadout = LOW;

  // Loop all pins switching one as OUTPUT
  for(int outputPinIndex=0; outputPinIndex < gameSwitchboardPinNum && gameSwitchboardPinActive1 == 0; outputPinIndex++) {

    // set the output pin LOW
    pinMode(gameSwitchboardPins[outputPinIndex], OUTPUT);
    digitalWrite(gameSwitchboardPins[outputPinIndex], LOW);

    for(int inputPinIndex = outputPinIndex+1; inputPinIndex < gameSwitchboardPinNum && gameSwitchboardPinActive1 == 0; inputPinIndex++) {

      // Don't switch to INPUT for the pin that is currently OUTPUT
      if(inputPinIndex == outputPinIndex) {
        continue;
      }

      pinMode(gameSwitchboardPins[inputPinIndex], INPUT_PULLUP);

      inputPinReadout = digitalRead(gameSwitchboardPins[inputPinIndex]);

      if (inputPinReadout == LOW) {
        gameSwitchboardPinActive1 = inputPinIndex;
        gameSwitchboardPinActive2 = outputPinIndex;

        #ifdef DEBUG
          Serial.print("game_switchboard_loop: Connection from IN ");
          Serial.print(gameSwitchboardPinActive1);
          Serial.print(" (");
          Serial.print(gameSwitchboardPins[gameSwitchboardPinActive1]);
          Serial.print(") to OUT ");
          Serial.print(gameSwitchboardPinActive2);
          Serial.print(" (");
          Serial.print(gameSwitchboardPins[gameSwitchboardPinActive2]);
          Serial.println(")");
        #endif
      }
    }

    // Set last loop pin to INPUT again
    pinMode(gameSwitchboardPins[outputPinIndex], INPUT_PULLUP);
  }

  // Connection found?
  if(gameSwitchboardPinActive1 > 0 || gameSwitchboardPinActive2 > 0) {
    matrixSetByArray(matrixPicSmileyPositive, 0,0, CRGB::Green);
  }
} /*}}} */

void game_master_loop() { /*{{{*/
  switch (state.current) {
    case 1 ... 9:
      game_intro_loop();
      break;
    case 10:
      game_choose();
      break;
    case 20:
      game_switchboard_reset();
      break;
    case 21 ... 39:
      game_switchboard_loop();
      break;
  }
} /*}}} */

void loop() /*{{{*/
{
  // advance state
  updateState();

  // Reset all LEDs
  if (ledsModified == 1) {
    ledsModified = 0;
    fill_solid(leds, numLeds, CRGB::Black);
  }
 
  // Run master loop
  game_master_loop();

  // If leds have been modified, show them
  if (ledsModified == 1) {
    FastLED.show();
  }
} /*}}}*/
