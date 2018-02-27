#include <Arduino.h>
#include <HardwareSerial.h>
#include <FastLED.h>
#include <PGMWrap.h>

extern HardwareSerial Serial;

//                                  +-----+
//                     +------------| USB |-------------+
//                     |            +-----+             |
//                     | [ ]D13/SCK/noDin   MISO/D12[ ] | -> arcadeBtnsLeds[2]
//                     | [ ]3.3V            MOSI/D11[ ]~| <- arcadeBtnsDin[1]
//                     | [ ]V.ref     ___     SS/D10[ ]~| -> arcadeBtnsLeds[1]
// PRESSURE_LED_PIN <- | [ ]A0       / N \        D9[ ]~| <- arcadeBtnsDin[0]
// PRESSURE_BTN_PIN -> | [ ]A1      /  A  \       D8[ ] | -> arcadeBtnsLeds[0]
// MATRIX_LED_PIN   <- | [ ]A2      \  N  /       D7[ ] | <- switchboardPins[5] => 64
// arcadeBtnsDin[2] -> | [ ]A3       \_0_/        D6[ ]~| <- switchboardPins[4] => 32
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

// Global game state
typedef struct State {
  unsigned int current;
  unsigned int next;
  unsigned long nextStateAtMsec;
  unsigned int score;
  unsigned int lifes;
} State;

State state = { .current = 0, .next = 0, .nextStateAtMsec = 0, .score = 0, .lifes = 3};

const int matrixDinPin = A2;
const int numLeds = 100;
const int matrixLedBrightness = 5;

// 0 = no LED update
// 1 = full clear + update
// 2 = no clear, just update
byte ledsModified = 0;

// LED array
CRGB leds[numLeds];

// Currently active game
int activeGame = 0;

// Total number of playable games
long numberOfGames = 2;

// Number of PINS used for switchboard game
const int gameSwitchboardPinNum = 6;

// PINs for switchbaord game
const int gameSwitchboardPins[] = { 2, 3, 4, 5, 6, 7 };

// Mapping of PIN to value
const int gameSwitchboardPinValues[] = { 2, 4, 8, 16, 32, 64 };

int gameSwitchboardPinActive1 = 0;
int gameSwitchboardPinActive2 = 0;

// Running game state for switchboard
typedef struct SwitchBoard {
  int activePin1;
  int activePin2;
  int number;
  unsigned long startMillis;
  unsigned long timeToSolveMillis;
} Switchboard;
Switchboard switchboard = { .activePin1 = 0, .activePin2 = 0, .number = 0, .startMillis = 0, .timeToSolveMillis = 4000 };

// Pins for the button state (digital in) and leds out (5v)
const uint8_t gameArcadeButtonPinsDin[] = { 9, 11, A3 };
const uint8_t gameArcadeButtonPinsLedOut[] = { 8, 10, 12 };

const uint8_t gameArcadeButtonStartColumnPos[] = { 0, 2, 5 };
const uint8_t gameArcadeButtonStartRowPos[] = { 5, 1, 5 };

const CRGB gameArcadeButtonPinsColors[] = { CRGB::Red, CRGB::Yellow, CRGB::Green };
const uint8_t gameArcadeButtonNumberOfButtons = 3;
unsigned long gameArcadeButtonTimer = 0;
unsigned long gameArcadeButtonShowColorMsec = 2000;
unsigned long gameArcadeButtonShowColorPauseMsec = 500;
uint8_t gameArcadeButtonState = 0;
uint8_t numberOfButtonPresses = 3;

uint8_t gameArcadePressedButton = 0;
uint8_t gameArcadePrevPressedButton = 0;
uint8_t gameArcadePressedSoFarIndex = 0;

// Maximum presses is 20
uint8_t buttonsToPress[20];

/* alpha {{{*/
int8_p alpha[36][13] PROGMEM = {
  {0,60,16,129,7,0,0,0,0,0,0,0,0},  // 0
  {0,32,240,1,0,0,0,0,0,0,0,0,0},   // 1
  {0,76,80,65,2,0,0,0,0,0,0,0,0},   // 2
  {0,68,80,129,2,0,0,0,0,0,0,0,0},  // 3
  {0,112,64,192,7,0,0,0,0,0,0,0,0}, // 4
  {0,116,80,129,4,0,0,0,0,0,0,0,0}, // 5
  {0,60,80,193,5,0,0,0,0,0,0,0,0},  // 6
  {0,76,64,1,6,0,0,0,0,0,0,0,0},    // 7
  {0,124,80,193,7,0,0,0,0,0,0,0,0}, // 8
  {0,116,80,129,7,0,0,0,0,0,0,0,0}, // 9
  {0,60,64,193,3,0,0,0,0,0,0,0,0},  // A
  {0,124,80,129,2,0,0,0,0,0,0,0,0}, // B
  {0,56,16,65,4,0,0,0,0,0,0,0,0},   // C
  {0,124,16,129,3,0,0,0,0,0,0,0,0}, // D
  {0,124,80,65,5,0,0,0,0,0,0,0,0},  // E
  {0,124,64,1,5,0,0,0,0,0,0,0,0},   // F
  {0,56,80,193,5,0,0,0,0,0,0,0,0},  // G
  {0,124,64,192,7,0,0,0,0,0,0,0,0}, // H
  {0,68,240,65,4,0,0,0,0,0,0,0,0},  // I
  {0,8,16,128,7,0,0,0,0,0,0,0,0},   // J
  {0,124,64,192,6,0,0,0,0,0,0,0,0}, // K
  {0,124,16,64,0,0,0,0,0,0,0,0,0},  // L
  {0,124,192,192,7,0,0,0,0,0,0,0,0},// M
  {0,124,224,192,7,0,0,0,0,0,0,0,0},// N
  {0,56,16,129,3,0,0,0,0,0,0,0,0},  // O
  {0,124,64,1,2,0,0,0,0,0,0,0,0},   // P
  {0,56,48,193,3,0,0,0,0,0,0,0,0},  // Q
  {0,124,96,65,3,0,0,0,0,0,0,0,0},  // R
  {0,36,80,129,4,0,0,0,0,0,0,0,0},  // S
  {0,64,240,1,4,0,0,0,0,0,0,0,0},   // T
  {0,120,16,192,7,0,0,0,0,0,0,0,0}, // U
  {0,112,48,0,7,0,0,0,0,0,0,0,0},   // V
  {0,124,96,192,7,0,0,0,0,0,0,0,0}, // W
  {0,108,64,192,6,0,0,0,0,0,0,0,0}, // X
  {0,96,112,0,6,0,0,0,0,0,0,0,0},   // Y
  {0,76,80,65,6,0,0,0,0,0,0,0,0}    // Z
};
/*}}}*/

/* pictures {{{*/

int8_p matrixPicBig3[13] PROGMEM = {0,0,96,155,109,182,217,230,159,127,0,0,0};
int8_p matrixPicBig2[13] PROGMEM = {0,0,224,140,119,158,217,102,159,57,0,0,0};
int8_p matrixPicBig1[13] PROGMEM = {0,0,96,140,113,254,249,103,128,1,0,0,0};
int8_p matrixPicSmileyPositive[13] PROGMEM = {252,248,119,249,230,251,239,191,249,229,254,241,3};
int8_p matrixPicSmileyNegative[13] PROGMEM = {252,248,183,249,245,247,223,127,253,230,254,241,3};
int8_p matrixPicPlug[13] PROGMEM = {0,2,12,16,64,0,1,14,56,224,0,1,0};
int8_p matrixPicBorder[13] PROGMEM = {255,7,24,96,128,1,6,24,96,128,1,254,15};
int8_p matrixPicMan[13] PROGMEM = {17,136,198,191,104,17,0,0,0,0,0,0,0};
int8_p matrixPicSkull[13] PROGMEM = {240,96,230,57,247,246,115,239,57,102,240,0,0};
int8_p matrixPicButton[13] PROGMEM = {14,124,240,193,7,14,0,0,0,0,0,0,0};
int8_p matrixPicQuestion[13] PROGMEM = {0,0,6,56,192,59,239,13,62,112,0,0,0};
/*}}}*/

void changeStateTo(const unsigned int nextState, const unsigned long nextStateInMsec) { /*{{{*/
  // Instant switch
  if(nextStateInMsec == 0 && state.next != nextState) {
    state.current = nextState;

    #ifdef DEBUG
      Serial.print("changeStateTo: current = ");  
      Serial.print(state.current);
      Serial.println(", at msec = INSTANT");
    #endif

    return;
  }

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

void matrixSetByArray(int8_p picture[], int startColumn, int startRow, CRGB color, int partialUpdate) /*{{{*/{

  // Loop through all elements
  for(int pByte = 0; pByte < 13; pByte++) {
    for(int bit = 0; bit < 8; bit++) {
      if (bitRead(picture[pByte], bit)) {
        int col = ((pByte * 8 + bit) + (10 * startColumn)) / 10; 
        int pos = (pByte * 8 + bit) + (10 * startColumn) + startRow;
        if (col > -1 && col < 10) {
          ledsModified = partialUpdate == 1 ? 2 : 1;
          leds[pos] = color;
        }
      }
    }
  }
} /*}}}*/

void matrixSetByArray(int8_p picture[], int startColumn, int startRow, CRGB color) /*{{{*/{
  matrixSetByArray(picture, startColumn, startRow, color, 0);
} /*}}}*/

void matrixSetByIndex(int alphaIndex, int startColumn, int startRow, CRGB color, int partialUpdate) /*{{{*/{
  matrixSetByArray(alpha[alphaIndex], startColumn, startRow, color, partialUpdate);
} /*}}}*/

void matrixSetByIndex(int alphaIndex, int startColumn, int startRow, CRGB color) /*{{{*/{
  matrixSetByIndex(alphaIndex, startColumn, startRow, color, 0);
} /*}}}*/

void showProgressBar(unsigned long progress) { /*{{{*/
  if (progress > 0) {
    // progress is 0 -> 10
    for (uint8_t pLedI = 1; pLedI <= progress; pLedI++) {
      leds[(pLedI - 1) * 10] = CRGB::DodgerBlue;
      leds[(pLedI - 1) * 10].fadeLightBy(200);
    }
  }
  ledsModified = 2;
} /*}}} */

void clearMatrix() { /*{{{*/
  fill_solid(leds, numLeds, CRGB::Black);
  FastLED.show();
} /*}}} */

void game_arcade_button_display_button(int buttonIndex, CRGB color) { /*{{{*/
  matrixSetByArray(matrixPicButton, gameArcadeButtonStartColumnPos[buttonIndex], gameArcadeButtonStartRowPos[buttonIndex], color); 
} /*}}} */

void matrix_setup() /*{{{*/{
  FastLED.addLeds<NEOPIXEL, matrixDinPin>(leds, numLeds);
  FastLED.setBrightness(matrixLedBrightness);
} /*}}}*/

void game_setup() {/*{{{*/
  randomSeed(analogRead(4));
  state.lifes = 3;
  state.score = 0;
  state.current = 1;
} /*}}}*/

void setup() /*{{{*/
{
  Serial.begin(9600);
  
  matrix_setup();
  game_setup();
}/*}}}*/

// STATE: 2 ... 9
void game_intro_loop() { /*{{{*/
  switch (state.current) {
    case 2:
      matrixSetByArray(matrixPicBig3, 0, 0, CRGB::Red);
      changeStateTo(3, 750);
      break;
    case 3:
      matrixSetByArray(matrixPicBig2, 0, 0, CRGB::Crimson);
      changeStateTo(4, 750);
      break;
    case 4:
      matrixSetByArray(matrixPicBig1, 0, 0, CRGB::Yellow);
      changeStateTo(10, 750);
      break;
  }
} /*}}}*/

// STATE: 10
void game_choose() { /*{{{*/
  activeGame = random(1, numberOfGames + 1); 
  // TEMP
  activeGame = 2;

  // Every game has 10 possible states
  changeStateTo((activeGame * 10) + 10, 1);

  #ifdef DEBUG
    Serial.print("game_choose: activeGame = ");
    Serial.println(activeGame);
  #endif
} /*}}} */

// STATE: 20
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
  matrixSetByIndex((switchboard.number/10), 1, 2, CRGB::Green, 1);
  matrixSetByIndex((switchboard.number%10), 5, 2, CRGB::Green, 1);
  matrixSetByArray(matrixPicPlug, 0, 0, CRGB::Yellow, 1);

  // Remember start time
  switchboard.startMillis = millis();

  // Set time the player has to solve the cahllenge in msec
  // TODO: dynamic:
  switchboard.timeToSolveMillis = 10000;

  // Start game loop
  changeStateTo(21, 1);
} /*}}} */

// STATE: 21+
void game_switchboard_loop() { /*{{{*/
  // Reset
  int correctFound = 0;
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

    // Correct connection found?
    if((gameSwitchboardPinActive1 == switchboard.activePin1 &&
       gameSwitchboardPinActive2 == switchboard.activePin2) ||
       (gameSwitchboardPinActive1 == switchboard.activePin2 &&
       gameSwitchboardPinActive2 == switchboard.activePin1)) {
      // CORRECT!
      correctFound = 1;
      changeStateTo(101, 1);
    }
  }

  // Test if time is up only if not correct connection found
  if (correctFound == 0) {
    if(millis() - switchboard.startMillis <= switchboard.timeToSolveMillis) {
      unsigned long progress = switchboard.timeToSolveMillis + switchboard.startMillis - millis();
      unsigned long progressLeds = map(progress, 0, switchboard.timeToSolveMillis, 10, 0);
      showProgressBar(progressLeds);
    } else {
      // Game lost!
      changeStateTo(100, 1);
    }
  }
} /*}}} */

// STATE: 30
void game_arcade_button_reset() { /*{{{*/

  // Set PINS
  for(uint8_t i = 0; i < gameArcadeButtonNumberOfButtons; i++) {
    pinMode(gameArcadeButtonPinsDin[i], INPUT_PULLUP);
    pinMode(gameArcadeButtonPinsLedOut[i], OUTPUT);
    digitalWrite(gameArcadeButtonPinsLedOut[i], LOW);
  }
  gameArcadePressedButton = 0;
  gameArcadePressedSoFarIndex = 0;
  gameArcadePrevPressedButton = 255;

  // How many presses neccessary?
  numberOfButtonPresses = 3 + int((state.score / 10));

  // Choose random buttons for every slot
  for(uint8_t i = 0; i < numberOfButtonPresses; i++) {
    buttonsToPress[i] = random(0, gameArcadeButtonNumberOfButtons);
    #ifdef DEBUG
      Serial.print("game_arcade_button_reset: must press button #");
      Serial.print(i);
      Serial.print(" = ");
      Serial.println(buttonsToPress[i]);
    #endif
  }

  // Mark the end by adding 255 as end marker
  buttonsToPress[numberOfButtonPresses] = 255;

  gameArcadeButtonTimer = 0;
  gameArcadeButtonState = 1;

  // Display task at hand
  changeStateTo(31, 1);
} /*}}} */

// STATE: 31
void game_arcade_button_show_task() { /*{{{*/
  CRGB color;

  if (millis() > gameArcadeButtonTimer) {

    // Pause or next button display?
    if (gameArcadeButtonState % 2 == 0) {
      game_arcade_button_display_button(buttonsToPress[gameArcadeButtonState/2] - 1, CRGB::Black);
      gameArcadeButtonTimer = millis() + gameArcadeButtonShowColorPauseMsec;

      #ifdef DEBUG
        Serial.println("game_arcade_button_show_task(): Pausing");
      #endif

    } else {
      color = gameArcadeButtonPinsColors[buttonsToPress[gameArcadeButtonState/2]];
      gameArcadeButtonTimer = millis() + gameArcadeButtonShowColorMsec;

      #ifdef DEBUG
        Serial.print("game_arcade_button_show_task(): Showing button #");
        Serial.print(gameArcadeButtonState/2);
        Serial.print(", button ID ");
        Serial.print(buttonsToPress[gameArcadeButtonState/2]);
        Serial.print(", color ");
        Serial.println(gameArcadeButtonPinsColors[buttonsToPress[gameArcadeButtonState/2]]);
      #endif

      // Fill the corresponding block of the button in the matrix
      game_arcade_button_display_button(buttonsToPress[gameArcadeButtonState/2], gameArcadeButtonPinsColors[buttonsToPress[gameArcadeButtonState/2]]);
    }

    gameArcadeButtonState++;

    // Showed all buttons?
    if (buttonsToPress[gameArcadeButtonState/2] == 255) {
      changeStateTo(32, gameArcadeButtonShowColorMsec);
    }

    ledsModified = 1;
  }

} /*}}} */

// STATE: 32
void game_arcade_button_detect_pressed() { /*{{{*/
  matrixSetByArray(matrixPicQuestion, 0, 0, CRGB::Yellow);

  uint8_t changeStateNext = 0;

  for(uint8_t i = 0; i < gameArcadeButtonNumberOfButtons; i++) {

    // Detect first pressed button
    if (digitalRead(gameArcadeButtonPinsDin[i]) == HIGH) {
      gameArcadePrevPressedButton = gameArcadeButtonPinsDin[i];
      digitalWrite(gameArcadeButtonPinsLedOut[i], HIGH);

      #ifdef DEBUG
        Serial.print("game_arcade_button_detect_pressed(): Button D");
        Serial.print(gameArcadeButtonPinsDin[i]);
        Serial.print(" is HIGH; gameArcadePrevPressedButton = ");
        Serial.println(gameArcadePrevPressedButton);
      #endif
    } else if (gameArcadePrevPressedButton == gameArcadeButtonPinsDin[i] && digitalRead(gameArcadeButtonPinsDin[i]) == LOW) {
      // If the pressed button gets depressed -> count it
      gameArcadePressedButton = gameArcadeButtonPinsDin[i];

      #ifdef DEBUG
        Serial.print("game_arcade_button_detect_pressed(): Must press D");
        Serial.print(gameArcadeButtonPinsDin[buttonsToPress[gameArcadePressedSoFarIndex]]);
        Serial.print(" as press ");
        Serial.print(gameArcadePressedSoFarIndex+1);
        Serial.print(" of ");
        Serial.print(numberOfButtonPresses);
        Serial.print("; current read button #");
        Serial.print(i);
        Serial.print(" (D");
        Serial.print(gameArcadeButtonPinsDin[i]);
        Serial.print(") = ");
        Serial.println(digitalRead(gameArcadeButtonPinsDin[i]));
      #endif

      if (gameArcadeButtonPinsDin[i] != gameArcadeButtonPinsDin[buttonsToPress[gameArcadePressedSoFarIndex]]) {
        // Did NOT press the correct button
        changeStateNext = 33;
      } else if (gameArcadePressedSoFarIndex == numberOfButtonPresses - 1) {
        // All buttons pressed correctly?
        changeStateNext = 34;
      }

      gameArcadePressedSoFarIndex++;
      gameArcadePrevPressedButton = 255;

    } else {
      // Power the LED down
      digitalWrite(gameArcadeButtonPinsLedOut[i], LOW);
    }

    // Change State?
    if (changeStateNext > 0) {
      // Turn off leds
      for(uint8_t i = 0; i < gameArcadeButtonNumberOfButtons; i++) {
        digitalWrite(gameArcadeButtonPinsLedOut[i], LOW);
      }
      changeStateTo(changeStateNext, 1);
    }
  }
} /*}}} */

// STATE: 31 ... 39
void game_arcade_button_loop() { /*{{{*/
  switch(state.current) {
    // Show all buttons one by one
    case 31:
      game_arcade_button_show_task();
      break;
    // Expect presses
    case 32:
      game_arcade_button_detect_pressed();
      break;
    // Wrong button pressed
    case 33:
      changeStateTo(100, 1);
      break;
    // All buttons correctly pressed
    case 34:
      changeStateTo(101, 1);
      break;
  }
} /*}}} */

// STATE: 110
void displayScore() { /*{{{*/
  matrixSetByArray(matrixPicBorder, 0, 0, CRGB::Grey);
  matrixSetByIndex((state.score/10), 1, 2, CRGB::Grey);
  matrixSetByIndex((state.score%10), 5, 2, CRGB::Grey);
} /*}}} */

// STATE: 1 ... 110
void game_master_loop() { /*{{{*/
  switch (state.current) {
    case 1:
      game_setup();
      changeStateTo(2, 1);
      break;
    case 2 ... 9:
      // 3 ... 2 ... 1 ... GO!
      game_intro_loop();
      break;
    case 10:
      // Choose a random game
      game_choose();
      break;
    case 20:
      // Generate fresh switchboard game
      game_switchboard_reset();
      break;
    case 21 ... 29:
      // Search for solution for switchboard
      game_switchboard_loop();
      break;
    case 30:
      // Arcade Button fresh game
      game_arcade_button_reset();
      break;
    case 31 ... 39:
      // Arcade Button game loop
      game_arcade_button_loop();
      break;
    case 100:
      // Game lost -> show smiley -> new game or game over

      // Must change state to 0 so that this case is not called again
      state.current = 0;

      // reduce life by one
      state.lifes--;
      clearMatrix();
      matrixSetByArray(matrixPicSmileyNegative, 0,0, CRGB::Crimson);

      #ifdef DEBUG
        Serial.print("lifes left = ");
        Serial.println(state.lifes);
      #endif

      if (state.lifes == 0) {
        // All lifes lost, game over
        changeStateTo(111, 1500);
      } else {
        // Show life lost and reset
        changeStateTo(110, 1500);
      }

      break;
    case 101:
      // Game won!
      Serial.println("SCORE!");
      state.score++;
      clearMatrix();
      matrixSetByArray(matrixPicSmileyPositive, 0,0, CRGB::Green);
      changeStateTo(102, 1);
      break;
    case 102:
      changeStateTo(2, 2000);
      break;
    case 110:
      // Show lifes and reset
      matrixSetByArray(matrixPicMan, 0, 0, CRGB::Yellow);
      matrixSetByIndex(state.lifes, 4, 3, CRGB::Green);
      changeStateTo(2, 2000);
      break;
    case 111:
      // All lifes lost ... Game over!
      matrixSetByArray(matrixPicSkull, 0, 0, CRGB::Red);
      changeStateTo(112, 2000);
      break;
    case 112:
      displayScore();
      changeStateTo(200, 8000);
      break;
    case 200:
      clearMatrix();
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
  if (ledsModified > 0) {
    FastLED.show();
  }
} /*}}}*/
