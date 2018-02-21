#include <Arduino.h>
#include <HardwareSerial.h>
#include <FastLED.h>
#include <PGMWrap.h>

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

/* alpha {{{*/
int8_p alpha[10][12] PROGMEM = {
  {0,60,16,129,7,0,0,0,0,0,0,0}, // 0
  {0,32,240,1,0,0,0,0,0,0,0,0}, // 1
  {0,76,80,65,2,0,0,0,0,0,0,0}, // 2
  {0,68,80,129,2,0,0,0,0,0,0,0}, // 3
  {0,112,64,192,7,0,0,0,0,0,0,0}, // 4
  {0,116,80,129,4,0,0,0,0,0,0,0}, // 5
  {0,60,80,193,5,0,0,0,0,0,0,0}, // 6
  {0,76,64,1,6,0,0,0,0,0,0,0}, // 7
  {0,124,80,193,7,0,0,0,0,0,0,0}, // 8
  {0,116,80,129,7,0,0,0,0,0,0,0}, // 9
};
/*}}}*/

/* pictures {{{*/
int8_p matrixPicBig3[12] PROGMEM = {0,0,96,155,109,182,217,230,159,127,0,0};
int8_p matrixPicBig2[12] PROGMEM = {0,0,224,140,119,158,217,102,159,57,0,0};
int8_p matrixPicBig1[12] PROGMEM = {0,0,96,140,113,254,249,103,128,1,0,0};
int8_p matrixPicRunner[12] PROGMEM = {0,76,32,2,11,60,236,147,12,4,0,0};
int8_p matrixPicSmileyPositive[12] PROGMEM = {252,248,119,249,230,251,239,191,249,229,254,241};
/*}}}*/

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

void matrixSetByArray(int8_p picture[], int startColumn, int startRow, CRGB color) /*{{{*/{
  ledsModified = 1;

  // Loop through all elements
  for(int pByte = 0; pByte < 12; pByte++) {
    for(int bit = 0; bit < 8; bit++) {
      if (bitRead(picture[pByte], bit)) {
        leds[(pByte * 8 + bit) + (10 * startColumn) + startRow] = color;
      }
    }
  }
} /*}}}*/

void matrixSetByIndex(int alphaIndex, int startColumn, int startRow, CRGB color) /*{{{*/{
  matrixSetByArray(alpha[alphaIndex], startColumn, startRow, color);
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
