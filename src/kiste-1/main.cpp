#include <Arduino.h>
#include <FastLED.h>
#include <PGMWrap.h>
#if defined (DEBUG) || defined (DEBUG2)
  #include <HardwareSerial.h>
  extern HardwareSerial Serial;
#endif

//                                   +-----+
//                      +------------| USB |-------------+
//                      |            +-----+             |
// arcadeBtnsLeds[2] <- | [ ]D13/SCK/noDin   MISO/D12[ ] | -> arcadeBtnsLeds[1]
//                      | [ ]3.3V            MOSI/D11[ ]~| -> arcadeBtnsLeds[0]
//                      | [ ]V.ref     ___     SS/D10[ ]~| <- arcadeBtnsDin[2]
// MATRIX_LED_PIN    <- | [ ]A0       / N \        D9[ ]~| <- arcadeBtnsDin[1]
// PRESSURE_BTN_PIN_L-> | [ ]A1      /  A  \       D8[ ] | <- arcadeBtnsDin[0]
// PRESSURE_BTN_PIN_R-> | [ ]A2      \  N  /       D7[ ] | <- switchboardPins[5] => 64 (ONE COUNTERCLOCKWISE OF TOP)
// PR_LED_PIN        <- | [ ]A3       \_0_/        D6[ ]~| <- switchboardPins[4] => 32
// LT  potiPins[0]   -> | [ ]A4/SDA                D5[ ]~| <- switchboardPins[3] => 16
// RT  potiPins[1]   -> | [ ]A5/SCL                D4[ ] | <- switchboardPins[2] => 8
// LB  potiPins[2]   -> | [ ]A6 (noDIn)       INT1/D3[ ]~| <- switchboardPins[1] => 4 (FIRST CLOCKWISE)
// RB  potiPins[3]   -> | [ ]A7 (noDIn)       INT0/D2[ ] | <- switchboardPins[0] => 2 (TOP)
//                      | [ ]5V                   GND[ ] |
//                      | [ ]RST                  RST[ ] |
//                      | [ ]GND   5V MOSI GND D0/RX1[ ] |
//                      | [ ]Vin   [ ] [ ] [ ] D1/TX1[ ] |
//                      |          [ ] [ ] [ ]           |
//                      |          MISO SCK RST          |
//                      | NANO-V3                        |
//                      +--------------------------------+
//
// Potis read 1 full counterclockwise and GAME_POTI_MAP_TO_MAX full clockwise
//
// Poti Cables / Scale color:
// --------------------------
// Poti 1: Yellow / Yellow
// Poti 2: Blue   / Red
// Poti 3: Orange / Green
// Poti 4: Green  / Blue
//
// Arcade switches are connected as follows (looking straight at the NO/NC pins from the front)
//
//                GND 
// LEDPinIn -> |___|___| -> GND
//               |   |
//               | - |
// Button Out <- | - |
// (NC)          |___|
//
// PRESSURE_BTN_PIN_L = Left button
// PRESSURE_BTN_PIN_R = Right Button
//
// GAMES:
//
// 1. Switchboard
// --------------
// The game will display a number which is the addition of the two ports to connect.
// eg. When 36 is shown connect D4 (Value 4) and D6 (Value 32) must be connected.
// Every 20 score the player gets one second less time for the connection.
//
// 2. Arcade Buttons
// -----------------
// A sequence of buttons will be displayed. 
// Press them in this order. Thinks the classical Senso/Simon game.
// Every 7 points (gameArcadeLongerChainEveryScore) one color to remember is added.
// Once the player gets more than 10 (2 * gameArcadeLongerChainEveryScore) points in the game the button LEDS 
// themself will also be used.
//
// 3. Potentiometer
// ----------------
// Dial the potentiometers in each corner to the corresponding value indicated by the number OF THE SAME COLOR.
// Once the player reached 10 points the position of the numbers will be randomized in 25% of all cases.
// For every 10 points the player will get one second less time to react.
//
// Difficulty settings:
// --------------------
// The top left poti can be used to set the difficulty (see set_difficulty())
// Pressing the right pressure release button allows the player to choose only ONE game which will repeat over and over.
//
// TODOS:
// - Highscore keeping

/* DEBUG SETTINGS {{{ */
// Cancel all things related to the pressure release game
// Set to 1 to ignore pressure release game
// Set to 255 to ONLY run the pressure release game
uint8_t pressure_release_game_flag = 0;

// Force a specific game to repeat over and over
// Set to 0 to run normally
uint8_t force_game_nr = 0;

// Overwrite time to solve
// Set to 0 to use normal times
unsigned long force_time_to_solve_msec = 0;
/*}}} */

/* GLOBAL GAME STATE {{{*/

// The Main entry point when the power is turned on or everything is reset
#define GAME_BOX_STATE_START 150 

// Startvalues
#define START_WITH_SCORE 0
#define START_WITH_LIVES 3
#define NUMBER_OF_GAMES 3

typedef struct State {
  uint8_t current;
  uint8_t next;
  unsigned long nextStateAtMsec;
  uint8_t score;
  uint8_t lifes;
  uint8_t difficulty;
} State;

State state = { .current = GAME_BOX_STATE_START, .next = GAME_BOX_STATE_START, .nextStateAtMsec = 1, .score = START_WITH_SCORE, .lifes = START_WITH_LIVES, .difficulty = 0};

// Currently active game
uint8_t activeGame = 0;

// If the user only wants one game...
uint8_t playOnlyGameNr = 0;

unsigned long previousProgress = 0;

/*}}}*/

/*{{{ PRESSURE RELEASE SETTINGS */
// Number of leds in the pr led strip
const uint8_t pressureReleaseNumLeds = 9;

// Pressure Release LEDs
CRGB pressureReleaseLeds[pressureReleaseNumLeds];

// Pins used for the re buttons LEFT and RIGHT
const uint8_t pressureReleasePinIn[] = { A1, A2 };

// Colors used for thread level indication
// 0 -> 4
const CHSV pressureReleaseThreadLevelColors[] = { CHSV(HUE_GREEN, 255, 255), CHSV(HUE_YELLOW, 255, 255), CHSV(HUE_ORANGE, 255, 255), CHSV(HUE_RED, 255, 255), CHSV(HUE_PINK, 255, 255) };

// Pin used for addressing the pr strip
const uint8_t pressureReleaseLedPin = A3;

// Brightness of led strip
const uint8_t pressureReleaseLedBrightness = 20;

// 1 = need redraw
uint8_t pressureReleaseLedsModified = 0;

// Wether the pressure release loop is run or not
uint8_t pressureReleaseIsTicking = 1;

// current threat level and direction
int8_t pressureReleaseCurrentLevel = 0;
int8_t pressureReleaseCurrentDirection = 1;

// Timers
unsigned long pressureReleaseTimerMsec;
unsigned long pressureReleaseNextStepMsec;
unsigned long pressureReleaseFirstStepMsec;

// Currently pressed button. When released will be used as "pressed button"
uint8_t pressureReleaseCurrentButton = 255;

#ifdef DEBUG2
  // Used to not spam the serial console
  uint8_t pressureReleaseDebugTicker = 0;
#endif

/*}}}*/

/* MATRIX SETTINGS {{{*/
const uint8_t matrixDinPin = A0;

// 100 Leds for display (10x10)
const uint8_t matrixNumLeds = 100;
const uint8_t matrixLedBrightness = 20;

// 0 = no LED update
// 1 = full clear + update
// 2 = no clear, just update
byte matrixLedsModified = 0;

// LED array
CRGB matrixLeds[matrixNumLeds];
/*}}}*/

/* SWITCHBOARD SETTINGS {{{*/
// Number of PINS used for switchboard game
const uint8_t gameSwitchboardPinNum = 6;

// PINs for switchbaord game
const uint8_t gameSwitchboardPins[] = { 2, 3, 4, 5, 6, 7 };

// Mapping of PIN to value
const uint8_t gameSwitchboardPinValues[] = { 2, 4, 8, 16, 32, 64 };

// How long has the player to keep the connection?
const uint32_t gameSwitchboardTimeToKeepConnectionMsec = 1500;

// Flags / Timer
uint8_t gameSwitchboardPinActive1 = 0;
uint8_t gameSwitchboardPinActive2 = 0;
uint32_t gameSwitchboardTimeCorrectConnection;

// Running game state for switchboard
typedef struct SwitchBoard {
  uint8_t activePin1;
  uint8_t activePin2;
  uint8_t number;
  unsigned long startMillis;
  unsigned long timeToSolveMillis;
} Switchboard;
Switchboard switchboard = { .activePin1 = 0, .activePin2 = 0, .number = 0, .startMillis = 0, .timeToSolveMillis = 4000 };

/*}}}*/

/* ARCADE BUTTON SETTINGS {{{*/
// How many buttons?
const uint8_t gameArcadeButtonNumberOfButtons = 3;

// Pins for the button state (digital in) and leds out (5v)
const uint8_t gameArcadeButtonPinsDin[] = { 8, 9, 10 };
const uint8_t gameArcadeButtonPinsLedOut[] = { 11, 12, 13 };

// Where to start displaying the button on the matrix
const uint8_t gameArcadeButtonStartColumnPos[] = { 0, 2, 5 };
const uint8_t gameArcadeButtonStartRowPos[] = { 5, 1, 5 };

// Colors of Buttons LEFT to RIGHT
const CRGB gameArcadeButtonPinsColors[] = { CRGB::Red, CRGB::Yellow, CRGB::Green };

// Timer settings
unsigned long gameArcadeButtonTimer = 0;

// How long to display a button and how long to pause in msec
unsigned long gameArcadeButtonShowColorMsec = 1000;
unsigned long gameArcadeButtonShowColorPauseMsec = 200;
unsigned long gameArcadeButtonTimeToSolveTask = 5000;

uint8_t gameArcadeButtonState = 0;
uint8_t gameArcadeLongerChainEveryScore = 7;

// Number of presses needed to solve this stage
uint8_t gameArcadeNumberOfButtonPresses = 3;

// Temporarystate vars
uint8_t gameArcadePressedButton = 0;
uint8_t gameArcadePrevPressedButton = 0;
uint8_t gameArcadePressedSoFarIndex = 0;
uint8_t gameArcadeButtonUseButtonLedsAlso = 0;

// Maximum presses is 20
uint8_t buttonsToPress[20];
/*}}}*/

/* POTI SETTINGS {{{*/
#define GAME_POTI_NUM_POTIS 4 
#define GAME_POTI_MAP_TO_MAX 6

// These are analog pins!
// From Left/Top to Bottom/Right
const uint8_t gamePotiPins[] = { 4, 5, 6, 7 };
uint8_t gamePotiChallenge[] = { 0, 0, 0, 0 };
CRGB gamePotiColors[] = { CRGB::Yellow, CRGB::Red, CRGB::Green, CRGB::Blue };
unsigned long gamePotiTimeToSolveMsec = 5000;
unsigned long gamePotiChallengeStartMsec;
uint16_t gamePotiReadings[GAME_POTI_NUM_POTIS];
uint8_t gamePotiReadingsIndex = 0;
uint8_t gamePotiCurrentValue[] = { 0, 0, 0, 0 };

// 0 = Top Left
// 1 = Top Right
// 2 = Bottom Left
// 3 = Bottom Right
uint8_t gamePotiHintStartColumn[] = { 0, 5, 0, 5 };
uint8_t gamePotiHintStartRow[] = { 5, 5, 0, 0 };
/*}}}*/

/* alpha {{{*/
int8_p alpha[10][13] PROGMEM = {
  {0,60,16,129,7,0,0,0,0,0,0,0,0},  // 0
  {0,32,240,1,0,0,0,0,0,0,0,0,0},   // 1
  {0,76,80,65,2,0,0,0,0,0,0,0,0},   // 2
  {0,68,80,129,2,0,0,0,0,0,0,0,0},  // 3
  {0,112,64,192,7,0,0,0,0,0,0,0,0}, // 4
  {0,116,80,129,4,0,0,0,0,0,0,0,0}, // 5
  {0,60,80,193,5,0,0,0,0,0,0,0,0},  // 6
  {0,76,64,1,6,0,0,0,0,0,0,0,0},    // 7
  {0,124,80,193,7,0,0,0,0,0,0,0,0}, // 8
  {0,116,80,129,7,0,0,0,0,0,0,0,0} // 9
};
/*}}}*/

/* pictures {{{*/
int8_p matrixPicBig3[13] PROGMEM = {0,0,96,155,109,182,217,230,159,127,0,0,0};
int8_p matrixPicBig2[13] PROGMEM = {0,0,224,140,119,158,217,102,159,57,0,0,0};
int8_p matrixPicBig1[13] PROGMEM = {0,0,96,140,113,254,249,103,128,1,0,0,0};
int8_p matrixPicSmileyPositive1[13] PROGMEM = {252,248,119,249,230,251,239,191,249,229,254,241,3};
int8_p matrixPicSmileyPositive2[13] PROGMEM = {252,216,55,121,228,241,199,31,249,228,246,241,3};
int8_p matrixPicSmileyPositive3[13] PROGMEM = {188,216,54,113,198,185,231,158,241,196,182,241,2};
int8_p matrixPicSmileyNegative[13] PROGMEM = {252,248,183,249,245,247,223,127,253,230,254,241,3};
int8_p matrixPicPlug[13] PROGMEM = {0,2,12,16,64,0,1,14,56,224,0,1,0};
int8_p matrixPicBorder[13] PROGMEM = {255,7,24,96,128,1,6,24,96,128,1,254,15};
int8_p matrixPicMan[13] PROGMEM = {17,136,198,191,104,17,0,0,0,0,0,0,0};
int8_p matrixPicSkull[13] PROGMEM = {240,96,230,57,247,246,115,239,57,102,240,0,0};
int8_p matrixPicButton[13] PROGMEM = {14,124,240,193,7,14,0,0,0,0,0,0,0};
int8_p matrixPicQuestion[13] PROGMEM = {0,0,6,56,192,22,219,13,62,112,0,0,0};
int8_p matrixPicPoti1[13] PROGMEM = {252,24,54,112,128,49,198,24,227,204,182,241,3};
int8_p matrixPicPoti2[13] PROGMEM = {252,216,54,115,140,49,198,24,224,192,134,241,3};
int8_p matrixPicPoti3[13] PROGMEM = {252,24,54,124,184,113,198,24,224,192,134,241,3};
int8_p matrixPicPoti4[13] PROGMEM = {252,24,54,112,128,49,198,25,238,240,134,241,3};
int8_p matrixPicIntro1[13] PROGMEM = {1,4,112,64,1,69,148,81,255,253,97,4,1};
int8_p matrixPicIntro2[13] PROGMEM = {1,4,48,192,0,35,204,176,223,126,49,132,0};
int8_p matrixPicArrowDown[13] PROGMEM = {0,32,192,128,3,255,255,239,0,3,8,0,0};
/*}}}*/

void change_state_to(const uint8_t nextState, const unsigned long nextStateInMsec) { /*{{{*/
  if (state.next != nextState) {
    state.nextStateAtMsec = millis() + nextStateInMsec;
    state.next = nextState;

    #ifdef DEBUG2
      Serial.print("[STATE]: next = ");  
      Serial.print(state.next);
      Serial.print(" @msec = ");
      Serial.println(state.nextStateAtMsec);
    #endif
  }
} /*}}} */

void update_state() { /*{{{*/
  if (state.nextStateAtMsec > 0 && state.nextStateAtMsec <= millis()) {
    state.current = state.next;
    state.nextStateAtMsec = 0;
    state.next = 0;

    #ifdef DEBUG2
      Serial.print("[STATE] current = ");
      Serial.println(state.current);
    #endif
  }
} /*}}} */

void matrix_set_picture(int8_p picture[], uint8_t startColumn, uint8_t startRow, CRGB color, uint8_t partialUpdate) /*{{{*/{

  // Loop through all elements
  for(uint8_t pByte = 0; pByte < 13; pByte++) {
    for(uint8_t bit = 0; bit < 8; bit++) {
      if (bitRead(picture[pByte], bit)) {
        uint8_t col = ((pByte * 8 + bit) + (10 * startColumn)) / 10; 
        uint8_t pos = (pByte * 8 + bit) + (10 * startColumn) + startRow;
        if (col >= 0 && col < 10) {
          matrixLedsModified = partialUpdate == 1 ? 2 : 1;
          matrixLeds[pos] = color;
        }
      }
    }
  }
} /*}}}*/

void matrix_set_picture(int8_p picture[], uint8_t startColumn, uint8_t startRow, CRGB color) /*{{{*/{
  matrix_set_picture(picture, startColumn, startRow, color, 0);
} /*}}}*/

void matrix_set_number(uint8_t alphaIndex, uint8_t startColumn, uint8_t startRow, CRGB color, uint8_t partialUpdate) /*{{{*/{
  matrix_set_picture(alpha[alphaIndex], startColumn, startRow, color, partialUpdate);
} /*}}}*/

void matrix_set_number(uint8_t alphaIndex, uint8_t startColumn, uint8_t startRow, CRGB color) /*{{{*/{
  matrix_set_number(alphaIndex, startColumn, startRow, color, 0);
} /*}}}*/

void show_progress_bar(unsigned long progress, uint8_t horizontal) { /*{{{*/
  if (progress > 0 && previousProgress != progress) {
    previousProgress = progress;

    // progress is 0 -> 10
    for (uint8_t pLedI = 1; pLedI <= progress; pLedI++) {
      if (horizontal == 0) {
        matrixLeds[(pLedI - 1) * 10] = CRGB::Red;
      } else {
        matrixLeds[90 + pLedI - 1] = CRGB::Red;
      }
    }
  }
  matrixLedsModified = 2;
} /*}}} */

void update_matrix_leds() {/*{{{*/
  FastLED[0].showLeds(matrixLedBrightness);
} /*}}} */

void update_pressure_release_leds() {/*{{{*/
  FastLED[1].showLeds(pressureReleaseLedBrightness);
} /*}}} */

void clear_matrix() { /*{{{*/
  fill_solid(matrixLeds, matrixNumLeds, CRGB::Black);
  update_matrix_leds();
} /*}}} */

void game_arcade_button_display_button(uint8_t buttonIndex, CRGB color) { /*{{{*/
  // Choose a random position
  uint8_t r = random(0, 3);
  matrix_set_picture(matrixPicButton, gameArcadeButtonStartColumnPos[r], gameArcadeButtonStartRowPos[r], color); 
} /*}}} */

void matrix_setup() /*{{{*/{
  FastLED.addLeds<NEOPIXEL, matrixDinPin>(matrixLeds, matrixNumLeds);
} /*}}}*/

void pressure_release_draw_state() { /*{{{*/
  uint8_t currentLit = pressureReleaseCurrentLevel + (pressureReleaseNumLeds/2);

  // Threat level
  // 432101234
  uint8_t threadLevel = abs(pressureReleaseCurrentLevel);

  // Clear and display current level
  fill_solid(pressureReleaseLeds, pressureReleaseNumLeds, CRGB::Black);
  pressureReleaseLeds[currentLit] = pressureReleaseThreadLevelColors[threadLevel];

  // Trigger draw in loop()
  pressureReleaseLedsModified = 1;
} /*}}} */

void pressure_release_button_handling() { /*{{{*/
  // LOW == pressed
  if (digitalRead(pressureReleasePinIn[0]) == LOW) {
    pressureReleaseCurrentButton = 0;
  } else if (digitalRead(pressureReleasePinIn[1]) == LOW) {
    // RIGHT BUTTON
    pressureReleaseCurrentButton = 1;
  } 

  // Button 0 released -> modify counter
  if ( digitalRead(pressureReleasePinIn[0]) == HIGH && pressureReleaseCurrentButton == 0 && pressureReleaseCurrentLevel < (pressureReleaseNumLeds/2)) {
    // LEFT button
    pressureReleaseCurrentLevel++; 

    #ifdef DEBUG2
      Serial.print("pressure_release_button_handling: Button #0 (LEFT) released");
    #endif

    pressure_release_draw_state();
    pressureReleaseCurrentButton = 255;
  }

  // Button 1 released -> modify counter
  if (digitalRead(pressureReleasePinIn[1]) == HIGH && pressureReleaseCurrentButton == 1 && pressureReleaseCurrentLevel > -(pressureReleaseNumLeds/2)) {
    pressureReleaseCurrentLevel--; 

    #ifdef DEBUG2
      Serial.print("pressure_release_button_handling: Button #1 (RIGHT) released");
    #endif

    pressure_release_draw_state();
    pressureReleaseCurrentButton = 255;
  }

  #ifdef DEBUG2
    if (pressureReleaseCurrentButton == 255) {
      Serial.print("pressure_release_button_handling: pressureReleaseCurrentLevel = ");
      Serial.println(pressureReleaseCurrentLevel);
    }
  #endif
} /*}}} */

void pressure_release_led_setup() { /*{{{*/
  FastLED.addLeds<NEOPIXEL, pressureReleaseLedPin>(pressureReleaseLeds, pressureReleaseNumLeds);
} /*}}} */

void pressure_release_setup() { /*{{{*/
  // Cancel if not wanted
  if (pressure_release_game_flag == 1) {
    // Clear PR leds
    fill_solid(pressureReleaseLeds, pressureReleaseNumLeds, CRGB::Black);
    pressureReleaseLedsModified = 1;
    return;
  }

  // 0000x0000
  // Pressure Release values from -4 to 4
  pressureReleaseCurrentLevel = 0;

  // In which direction does the pressure move?
  // -1 left
  // +1 right
  pressureReleaseCurrentDirection = random(0, 2) == 0 ? -1 : 1;

  // remember starttime
  pressureReleaseTimerMsec = millis();

  // When does a new step occur?
  pressureReleaseFirstStepMsec = 2000;
  pressureReleaseNextStepMsec = 8000;

  // Not running
  pressureReleaseIsTicking = 0;

  // Set pins for buttons
  pinMode(pressureReleasePinIn[0], INPUT_PULLUP);
  pinMode(pressureReleasePinIn[1], INPUT_PULLUP);

  #ifdef DEBUG2
    Serial.print("pressure_release_setup: Current level 0 with direction "); 
    Serial.println(pressureReleaseCurrentDirection);
  #endif

  pressure_release_draw_state();
} /*}}} */

void display_score() { /*{{{*/
  matrix_set_picture(matrixPicBorder, 0, 0, CRGB::Grey);
  matrix_set_number((state.score/10), 1, 2, CRGB::Grey);
  matrix_set_number((state.score%10), 5, 2, CRGB::Grey);
} /*}}} */

void game_setup() {/*{{{*/
  randomSeed(analogRead(4));
  state.lifes = START_WITH_LIVES;
  state.score = START_WITH_SCORE;

  // Start at "press any key"
  state.current = GAME_BOX_STATE_START;
} /*}}}*/

void game_over_or_next_game(uint8_t showPicture) { /*{{{*/
  // Must change state to 0 so that this case is not called again
  state.current = 0;

  // reduce life by one
  state.lifes--;

  // Show Smiley
  if (showPicture) {
    clear_matrix();
    matrix_set_picture(matrixPicSmileyNegative, 0,0, CRGB::Crimson);
  }

  // Turn off pressure release game
  pressureReleaseIsTicking = 0;

  #ifdef DEBUG2
    Serial.print("Lifes left = ");
    Serial.println(state.lifes);
  #endif

  if (state.lifes == 0) {
    // All lifes lost, game over
    change_state_to(111, 1500);
  } else {
    // Show life lost and reset
    change_state_to(110, 1500);
  }
} /*}}} */

void press_any_button_to_start_game() { /*{{{*/
  // Read arcade buttons
  for(uint8_t i=0; i < gameArcadeButtonNumberOfButtons; i++) {
    pinMode(gameArcadeButtonPinsDin[i], INPUT_PULLUP);

    if (digitalRead(gameArcadeButtonPinsDin[i]) == HIGH) {
      #ifdef DEBUG2
        Serial.print("[press any key] Arcade button ");
        Serial.print(i);
        Serial.println(" pressed!");
      #endif
      change_state_to(1, 1);
      break;
    }
  }
} /*}}} */

void set_difficulty(uint8_t showDifficulty) { /*{{{*/
  CRGB diffColor = CRGB::Yellow;

  // POTI 0 (Top Left) can be used to set difficulty to 1 -> 6
  state.difficulty = map(analogRead(gamePotiPins[0]), 0, 1023, 1, GAME_POTI_MAP_TO_MAX + 1);

  // Reset relevant vars
  pressure_release_game_flag = 0;
  force_game_nr = 0;

  // Show difficulty on matrix display?
  if (showDifficulty == 1) {
    matrix_set_number(state.difficulty, 0, 5, diffColor);
  }

  // DIFFICULTY 1: Only arcade button game, no PR game
  // DIFFICULTY 2: Arcade button game, poti game, no PR game (see game_choose())
  // DIFFICULTY 3: All games, no PR game
  // DIFFICULTY 4: Only arcade button game + PR game
  // DIFFICULTY 5: Arcade button game, poti game and PR game (see game_choose())
  // DIFFICULTY 6: All games and PR game
  switch(state.difficulty) {
    case 1:
      force_game_nr = 2;
      pressure_release_game_flag = 1;
      break;
    case 2:
      pressure_release_game_flag = 1;
      break;
    case 3:
      pressure_release_game_flag = 1;
      break;
    case 4:
      force_game_nr = 2;
      break;
  }

  // The right PR button can be used to set a SINGLE game
  if (digitalRead(pressureReleasePinIn[1]) == LOW) {
    if (playOnlyGameNr >= NUMBER_OF_GAMES) {
      playOnlyGameNr = 0;
    } else {
      pressure_release_game_flag = 0;
      playOnlyGameNr++;
    }
  }
} /*}}} */

void display_intro_picture() { /*{{{*/
  switch(playOnlyGameNr) {
    case 1:
      // Only switchboard game
      clear_matrix();
      matrix_set_picture(matrixPicPlug, 0, 0, CRGB::Yellow);
      break;
    case 2:
      // Only arcade game (same as poti setting 1) 
      clear_matrix();
      matrix_set_picture(matrixPicIntro1, 0, 0, CRGB::Yellow);
      break;
    case 3:
      // Only poti game
      clear_matrix();
      matrix_set_picture(matrixPicPoti1, 0, 0, CRGB::Yellow);
      break;
    case 0:
      if (state.current == GAME_BOX_STATE_START) {
        matrix_set_picture(matrixPicIntro1, 0, 0, CRGB::Green);
      } else {
        matrix_set_picture(matrixPicIntro2, 0, 0, CRGB::Green);
      }
  }
} /*}}} */
// STATE: 2 ... 9
void game_intro_loop() { /*{{{*/
  switch (state.current) {
    case 2:
      if (state.next == 0) {
        matrix_set_picture(matrixPicBig3, 0, 0, CRGB::Red);
        change_state_to(3, 750);
      }
      break;
    case 3:
      if (state.next == 0) {
        matrix_set_picture(matrixPicBig2, 0, 0, CRGB::Crimson);
        change_state_to(4, 750);
      }
      break;
    case 4:
      if (state.next == 0) {
        matrix_set_picture(matrixPicBig1, 0, 0, CRGB::Green);
        change_state_to(10, 750);
      }
      break;
  }
} /*}}}*/

// STATE: 10
void game_choose() { /*{{{*/

  if (state.difficulty == 2 || state.difficulty == 5) {
    // On difficulty 2 and 5 choose only arcade button (2) + poti game (3)
    activeGame = random(2, 3 + 1); 
  } else if (force_game_nr > 0) {
    // force a single game
    activeGame = force_game_nr;
  } else {
    activeGame = random(1, NUMBER_OF_GAMES + 1); 
  }

  // User want only one game
  if (playOnlyGameNr > 0) {
    activeGame = playOnlyGameNr;
  }

  #ifdef DEBUG2
    Serial.print("state.difficulty = ");
    Serial.print(state.difficulty);
    Serial.print(", force_game_nr = ");
    Serial.print(force_game_nr);
    Serial.print(", playOnlyGameNr = ");
    Serial.print(playOnlyGameNr);
    Serial.print(", activeGame = ");
    Serial.println(activeGame);
  #endif

  // Every game has 10 possible states
  change_state_to((activeGame * 10) + 10, 1);

  #ifdef DEBUG2
    Serial.print("game_choose: activeGame = ");
    Serial.println(activeGame);
  #endif
} /*}}} */

// STATE: 20 (GAME 1: SWITCHBOARD)
void game_switchboard_reset() { /*{{{*/
  switchboard.activePin1 = random(0, gameSwitchboardPinNum);

  do {
    switchboard.activePin2 = random(0, gameSwitchboardPinNum);
  } while(switchboard.activePin1 == switchboard.activePin2);

  switchboard.number = gameSwitchboardPinValues[switchboard.activePin1] + gameSwitchboardPinValues[switchboard.activePin2];

  #ifdef DEBUG2
    Serial.print("game_switchboard_reset: activePin1 = ");
    Serial.print(switchboard.activePin1);
    Serial.print(", activePin2 = ");
    Serial.print(switchboard.activePin2);
    Serial.print(", number = ");
    Serial.println(switchboard.number);
  #endif

  // Reset all Pins as INPUTs
  for (uint8_t i=0; i < gameSwitchboardPinNum; i++) {
    pinMode(gameSwitchboardPins[i], INPUT_PULLUP);
  }
  gameSwitchboardPinActive1 = 0;
  gameSwitchboardPinActive2 = 0;
  gameSwitchboardTimeCorrectConnection = 0;

  // Display target number
  matrix_set_number((switchboard.number/10), 1, 2, CRGB::Green, 1);
  matrix_set_number((switchboard.number%10), 5, 2, CRGB::Green, 1);
  matrix_set_picture(matrixPicPlug, 0, 0, CRGB::Yellow, 1);

  // Remember start time
  switchboard.startMillis = millis();

  // Set time the player has to solve the challenge in msec
  // Every 20 score  = -1 sec
  if (force_time_to_solve_msec > 0 && force_game_nr > 0) {
    switchboard.timeToSolveMillis = force_time_to_solve_msec;
  } else {
    switchboard.timeToSolveMillis = 10000 - (state.score * 50);
    // Min time = 7 sec
    if (switchboard.timeToSolveMillis < 7000) {
      switchboard.timeToSolveMillis = 7000;
    }
  }

  // Start game loop
  change_state_to(21, 1);
} /*}}} */

// STATE: 21+
void game_switchboard_loop() { /*{{{*/
  // Reset
  uint8_t inputPinReadout = LOW;

  // Loop all pins switching one as OUTPUT
  for(uint8_t outputPinIndex=0; outputPinIndex < gameSwitchboardPinNum && gameSwitchboardPinActive1 == 0; outputPinIndex++) {

    // set the output pin LOW
    pinMode(gameSwitchboardPins[outputPinIndex], OUTPUT);
    digitalWrite(gameSwitchboardPins[outputPinIndex], LOW);

    for(uint8_t inputPinIndex = outputPinIndex+1; inputPinIndex < gameSwitchboardPinNum && gameSwitchboardPinActive1 == 0; inputPinIndex++) {
      // Don't switch to INPUT for the pin that is currently OUTPUT
      if(inputPinIndex == outputPinIndex) {
        continue;
      }

      pinMode(gameSwitchboardPins[inputPinIndex], INPUT_PULLUP);

      inputPinReadout = digitalRead(gameSwitchboardPins[inputPinIndex]);

      if (inputPinReadout == LOW) {
        gameSwitchboardPinActive1 = inputPinIndex;
        gameSwitchboardPinActive2 = outputPinIndex;

        #ifdef DEBUG2
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

      // First time the correct connection is done?
      if(gameSwitchboardTimeCorrectConnection == 0) {
        gameSwitchboardTimeCorrectConnection = millis();
      } else if(gameSwitchboardTimeCorrectConnection + gameSwitchboardTimeToKeepConnectionMsec <= millis()) {
        // Player has kept the connection long enough
        change_state_to(101, 1);
        return;
      }
    } else {
      // Connection wrong
      gameSwitchboardTimeCorrectConnection = 0;
    }
  } else {
    // No connection
    gameSwitchboardTimeCorrectConnection = 0;
  }

  // Test if time is up only if not correct connection found
  if (gameSwitchboardTimeCorrectConnection == 0) {
    gameSwitchboardPinActive1 = 0;
    gameSwitchboardPinActive2 = 0;

    if(millis() - switchboard.startMillis <= switchboard.timeToSolveMillis) {
      unsigned long progress = switchboard.timeToSolveMillis + switchboard.startMillis - millis();
      unsigned long progressLeds = map(progress, 0, switchboard.timeToSolveMillis, 10, 0);
      show_progress_bar(progressLeds, 0);
    } else {
      // Game lost!
      change_state_to(100, 1);
    }
  }
} /*}}} */

// STATE: 30 (GAME 2: ARCADE BUTTON)
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
  gameArcadeButtonUseButtonLedsAlso = 0;

  // How many presses necessary?
  gameArcadeNumberOfButtonPresses = 3 + int((state.score / gameArcadeLongerChainEveryScore));

  // Use arcade buttons?
  if (state.score > gameArcadeLongerChainEveryScore * 2) {
    gameArcadeButtonUseButtonLedsAlso = 1;
  }

  // Choose random buttons for every slot
  for(uint8_t i = 0; i < gameArcadeNumberOfButtonPresses; i++) {
    buttonsToPress[i] = random(0, gameArcadeButtonNumberOfButtons);

    // Use button LED instead of matrix? Add 10 to mark that!
    // Occurs with a 25% chance:
    if (gameArcadeButtonUseButtonLedsAlso && random(0, 4) == 1) {
      buttonsToPress[i] = buttonsToPress[i] + 10;
    }

    #ifdef DEBUG2
      Serial.print("game_arcade_button_reset: Must press button #");
      Serial.print(i);
      Serial.print(" = ");
      Serial.println(buttonsToPress[i]);
    #endif
  }

  // Mark the end by adding 255 as end marker
  buttonsToPress[gameArcadeNumberOfButtonPresses] = 255;

  gameArcadeButtonTimer = 1;
  gameArcadeButtonState = 1;

  // Show intro then task
  change_state_to(38, 1);
} /*}}} */

// STATE: 31
void game_arcade_button_show_task() { /*{{{*/
  CRGB color;

  if (gameArcadeButtonTimer > 0 && millis() > gameArcadeButtonTimer) {

    // Pause or next button display?
    if (gameArcadeButtonState % 2 == 0) {
      // Clearing display / Button LEDS
      game_arcade_button_display_button(buttonsToPress[gameArcadeButtonState/2] - 1, CRGB::Black);
      gameArcadeButtonTimer = millis() + gameArcadeButtonShowColorPauseMsec;

      // Turn off button LEDS
      for (uint8_t i = 0; i < gameArcadeButtonNumberOfButtons; i++) {
        digitalWrite(gameArcadeButtonPinsLedOut[i], LOW);
      }

      #ifdef DEBUG2
        Serial.println("game_arcade_button_show_task(): Pausing");
      #endif

    } else {
      color = gameArcadeButtonPinsColors[buttonsToPress[gameArcadeButtonState/2]];
      gameArcadeButtonTimer = millis() + gameArcadeButtonShowColorMsec;

      #ifdef DEBUG2
        Serial.print("game_arcade_button_show_task(): Showing button #");
        Serial.print(gameArcadeButtonState/2);
        Serial.print(", button ID ");
        Serial.print(buttonsToPress[gameArcadeButtonState/2]);
        Serial.print(", color ");
        Serial.println(gameArcadeButtonPinsColors[buttonsToPress[gameArcadeButtonState/2]]);
      #endif

      if (buttonsToPress[gameArcadeButtonState/2] >= 10) {
        // Light the button LED
        digitalWrite(gameArcadeButtonPinsLedOut[buttonsToPress[gameArcadeButtonState/2] - 10], HIGH);
      } else {
        // Fill the corresponding block of the button in the matrix
        game_arcade_button_display_button(buttonsToPress[gameArcadeButtonState/2], gameArcadeButtonPinsColors[buttonsToPress[gameArcadeButtonState/2]]);
      }
    }

    gameArcadeButtonState++;

    // Showed all buttons?
    if (buttonsToPress[gameArcadeButtonState/2] == 255) {
      gameArcadeButtonTimer = 0;
      change_state_to(32, gameArcadeButtonShowColorMsec);
    }

    matrixLedsModified = 1;
  }

} /*}}} */

// STATE: 32
void game_arcade_button_detect_pressed() { /*{{{*/
  uint8_t changeStateNext = 0;

  // On first call remember start msecs and show ? mark
  if (gameArcadeButtonTimer == 0) {
    matrix_set_picture(matrixPicQuestion, 0, 0, CRGB::Yellow);
    gameArcadeButtonTimer = millis();
  }

  for(uint8_t i = 0; i < gameArcadeButtonNumberOfButtons; i++) {

    uint8_t pressMe = buttonsToPress[gameArcadePressedSoFarIndex];

    // Marker for button led? Calculate the real button
    if (pressMe >= 10) {
      pressMe = pressMe - 10;
    }

    // Detect first pressed button
    if (digitalRead(gameArcadeButtonPinsDin[i]) == HIGH) {
      gameArcadePrevPressedButton = gameArcadeButtonPinsDin[i];
      digitalWrite(gameArcadeButtonPinsLedOut[i], HIGH);

      #ifdef DEBUG2
        Serial.print("game_arcade_button_detect_pressed(): Button D");
        Serial.print(gameArcadeButtonPinsDin[i]);
        Serial.print(" is HIGH; gameArcadePrevPressedButton = ");
        Serial.println(gameArcadePrevPressedButton);
      #endif
    } else if (gameArcadePrevPressedButton == gameArcadeButtonPinsDin[i] && digitalRead(gameArcadeButtonPinsDin[i]) == LOW) {
      // If the pressed button gets depressed -> count it
      gameArcadePressedButton = gameArcadeButtonPinsDin[i];

      #ifdef DEBUG2
        Serial.print("game_arcade_button_detect_pressed(): Must press D");
        Serial.print(gameArcadeButtonPinsDin[pressMe]);
        Serial.print(" as press #");
        Serial.print(gameArcadePressedSoFarIndex+1);
        Serial.print(" of ");
        Serial.print(gameArcadeNumberOfButtonPresses);
        Serial.print("; current read button #");
        Serial.print(i);
        Serial.print(" (D");
        Serial.print(gameArcadeButtonPinsDin[i]);
        Serial.print(") = ");
        Serial.println(digitalRead(gameArcadeButtonPinsDin[i]));
      #endif

      if (gameArcadeButtonPinsDin[i] != gameArcadeButtonPinsDin[pressMe]) {
        // Did NOT press the correct button
        changeStateNext = 33;
      } else if (gameArcadePressedSoFarIndex == gameArcadeNumberOfButtonPresses - 1) {
        // All buttons pressed correctly
        changeStateNext = 34;
      }

      gameArcadePressedSoFarIndex++;
      gameArcadePrevPressedButton = 255;

    } else {
      // Power the LED down
      digitalWrite(gameArcadeButtonPinsLedOut[i], LOW);
    }

    // Progress bar
    unsigned long progressLedsCount = map(gameArcadeButtonTimeToSolveTask + gameArcadeButtonTimer - millis(), 0, gameArcadeButtonTimeToSolveTask, 10, 0);
    show_progress_bar(progressLedsCount, 0);

    // Time over
    if (progressLedsCount >= 10) {
      changeStateNext = 33;
    }

    // Change State
    if (changeStateNext > 0) {
      // Turn off leds
      for(uint8_t i = 0; i < gameArcadeButtonNumberOfButtons; i++) {
        digitalWrite(gameArcadeButtonPinsLedOut[i], LOW);
      }
      change_state_to(changeStateNext, 1);
    }
  }
} /*}}} */

// STATE: 31 ... 39
void game_arcade_button_loop() { /*{{{*/
  switch(state.current) {
    case 31:
      // Show all buttons one by one
      game_arcade_button_show_task();
      break;
    case 32:
      // Expect presses
      game_arcade_button_detect_pressed();
      break;
    case 33:
      // Wrong button pressed
      change_state_to(100, 1);
      break;
    case 34:
      // All buttons correctly pressed
      change_state_to(101, 1);
      break;
  }
} /*}}} */

// STATE: 40 (GAME 3: POTIS)
void game_poti_reset() { /*{{{*/

  if (state.next == 0) {
    // Reset pins and generate new task
    for(uint8_t i; i < GAME_POTI_NUM_POTIS; i++) {
      // Since we use pins A4-A7 as 4-7 in the config we must calculate the correct pin (18 - 21)
      pinMode(14 + gamePotiPins[i], INPUT_PULLUP);

      // Set target number
      gamePotiChallenge[i] = int(random(1, GAME_POTI_MAP_TO_MAX + 1));

      // Set the current value
      // Set the sum
      gamePotiCurrentValue[i] = 0;
      gamePotiReadings[i] = 0;

      #ifdef DEBUG2
        Serial.print("game_poti_reset: Poti #");
        Serial.print(i);
        Serial.print(" = ");
        Serial.println(gamePotiChallenge[i]);
      #endif

      // How much time does the player get to solve it, msec
      if (force_time_to_solve_msec > 0) {
        gamePotiTimeToSolveMsec = force_time_to_solve_msec;
      } else {
        // Every 10 points/score loose one second of reaction time
        // with a lower limit of 6000 msec
        gamePotiTimeToSolveMsec = 15000 - int((state.score * 100));
        if (gamePotiTimeToSolveMsec < 8000) {
          gamePotiTimeToSolveMsec = 8000;
        }
      }
    }
  }

  change_state_to(41, 1);
} /*}}} */

// STATE: 42
void game_poti_show_challenge() { /*{{{*/
  if (state.next == 0) {

    uint8_t displayInColumn[GAME_POTI_NUM_POTIS];
    uint8_t displayInRow[GAME_POTI_NUM_POTIS];

    // Default: display the number in the corner of the poti
    for(uint8_t i = 0; i < GAME_POTI_NUM_POTIS; i++) {
      displayInColumn[i] = gamePotiHintStartColumn[i];
      displayInRow[i] = gamePotiHintStartRow[i];
    }

    // In 25% of cases with score >= 10 randomize position
    if (state.score >= 10 && random(0,5) == 0) {
      for (uint8_t i = 0; i < GAME_POTI_NUM_POTIS - 1; i++) {
        uint8_t targetPos = random(0, GAME_POTI_NUM_POTIS - i);

        uint8_t temp = displayInColumn[i];
        displayInColumn[i] = displayInColumn[targetPos];
        displayInColumn[targetPos] = temp;

        temp = displayInRow[i];
        displayInRow[i] = displayInRow[targetPos];
        displayInRow[targetPos] = temp;
      }
    }

    for(uint8_t i = 0; i < GAME_POTI_NUM_POTIS; i++) {

      #ifdef DEBUG2
       Serial.print("Showing poti #");
       Serial.print(i);
       Serial.print(" (");
       Serial.print(gamePotiChallenge[i]);
       Serial.print(")");
       Serial.print(" at column ");
       Serial.print(gamePotiHintStartColumn[i]);
       Serial.print(", row ");
       Serial.println(gamePotiHintStartRow[i]);
      #endif

      matrix_set_number(gamePotiChallenge[i], displayInColumn[i], displayInRow[i], gamePotiColors[i]);
      matrixLedsModified = 2;
    }

    gamePotiChallengeStartMsec = millis();
    change_state_to(43, 1);
  }
} /*}}} */

// STATE: 43
void game_poti_detect_potis() { /*{{{*/
  #ifdef DEBUG2
    Serial.print("game_poti_detect_potis: Current/Target = ");
  #endif

  // Read all potis 10 times and remember the values
  for(uint8_t potiIndex = 0; potiIndex < GAME_POTI_NUM_POTIS; potiIndex++) {
    gamePotiReadings[potiIndex] = 0;

    for(uint8_t readings = 0; readings < 10; readings++) {
      gamePotiReadings[potiIndex] += analogRead(gamePotiPins[potiIndex]);
    }

    // Take the average
    gamePotiCurrentValue[potiIndex] = map(gamePotiReadings[potiIndex]/10, 0, 1023, 1, GAME_POTI_MAP_TO_MAX + 1);
    //gamePotiCurrentValue[potiIndex] = map(gamePotiReadings[potiIndex]/10, 65, 962, 1, GAME_POTI_MAP_TO_MAX + 1);

    #ifdef DEBUG2
      Serial.print("#");
      Serial.print(potiIndex);
      Serial.print(" (A");
      Serial.print(gamePotiPins[potiIndex]);
      Serial.print(")");
      Serial.print("=");
      Serial.print(gamePotiCurrentValue[potiIndex]);
      Serial.print("/");
      Serial.print(gamePotiChallenge[potiIndex]);
      Serial.print(" (Abs: ");
      Serial.print(analogRead(gamePotiPins[potiIndex]));
      Serial.print("), ");
    #endif
  }

  #ifdef DEBUG2
    Serial.println("");
  #endif
} /*}}} */

// STATE: 43 (2)
void game_poti_check_solution() { /*{{{*/
  uint8_t correctPotis = 0;

  // Check all current poti values for correctness
  for(uint8_t potiIndex = 0; potiIndex < GAME_POTI_NUM_POTIS; potiIndex++) {
    if (gamePotiCurrentValue[potiIndex] == gamePotiChallenge[potiIndex]) {
      correctPotis++;
    }
  }

  // All correct?
  if (correctPotis == GAME_POTI_NUM_POTIS) {
    change_state_to(101, 1);
  } else {
    // Progress bar
    unsigned long progressLedsCount = map(gamePotiTimeToSolveMsec + gamePotiChallengeStartMsec - millis(), 0, gamePotiTimeToSolveMsec, 10, 0);
    show_progress_bar(progressLedsCount, 1);

    if (progressLedsCount >= 10) {
      // Timout! Loose game
      change_state_to(100, 1);
    }
  }
} /*}}} */

// STATE: 1 ... 110
void game_master_loop() { /*{{{*/
  switch (state.current) {
    case 1:
      game_setup();
      pressure_release_setup();
      change_state_to(2, 1);
      break;
    case 2 ... 9:
      // 3 ... 2 ... 1 ... GO!
      game_intro_loop();
      break;
    case 10:
      // Choose a random game if the PR game is not the only one wanted
      if (pressure_release_game_flag < 255 || playOnlyGameNr > 0) {
        game_choose();
      }
      pressureReleaseIsTicking = 1;
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
    case 31 ... 35:
      // Arcade Button game loop
      game_arcade_button_loop();
      break;
    case 38:
      if (state.next == 0) {
        matrix_set_picture(matrixPicIntro1, 0, 0, CRGB::Yellow);
        change_state_to(31, 1500);
      }
      break;
    case 40:
      // Poti fresh game
      game_poti_reset();
      break;
    case 41:
      // Show poti picture
      if (state.next == 0) {
        // there are 4 pictures so ...
        switch(random(1,5)) {
          case 1:
            matrix_set_picture(matrixPicPoti1, 0, 0, CRGB::Yellow);
            break;
          case 2:
            matrix_set_picture(matrixPicPoti2, 0, 0, CRGB::Yellow);
            break;
          case 3:
            matrix_set_picture(matrixPicPoti3, 0, 0, CRGB::Yellow);
            break;
          case 4:
            matrix_set_picture(matrixPicPoti4, 0, 0, CRGB::Yellow);
            break;
        }
      }
      change_state_to(42, 2000);
      break;
    case 42:
      game_poti_show_challenge();
      break;
    case 43:
      game_poti_detect_potis();
      game_poti_check_solution();
      break;
    case 100:
      // Game lost -> show smiley -> new game or game over
      game_over_or_next_game(1);
      break;
    case 101:
      // Game won!
      state.score++;

      clear_matrix();

      // Show smiley (1, 2, 3)
      switch(random(1,4)) {
        case 1:
          matrix_set_picture(matrixPicSmileyPositive1, 0,0, CRGB::Green);
          break;
        case 2:
          matrix_set_picture(matrixPicSmileyPositive2, 0,0, CRGB::Green);
          break;
        case 3:
          matrix_set_picture(matrixPicSmileyPositive3, 0,0, CRGB::Green);
          break;
      }

      change_state_to(102, 1);
      break;
    case 102:
      // Restart game:
      change_state_to(2, 2000);
      break;
    case 110:
      // Show lifes and reset
      pressure_release_setup();
      matrix_set_picture(matrixPicMan, 0, 0, CRGB::Yellow);
      matrix_set_number(state.lifes, 5, 3, CRGB::Green);
      change_state_to(2, 2000);
      break;
    case 111:
      // All lifes lost ... Game over!
      matrix_set_picture(matrixPicSkull, 0, 0, CRGB::Red);
      change_state_to(112, 2000);
      break;
    case 112:
      // Display score and end game
      display_score();
      change_state_to(200, 8000);
      break;
    case GAME_BOX_STATE_START:
      // Box intro 1
      pressureReleaseIsTicking = 0;
      if (state.next == 0) {
        set_difficulty(1);
        display_intro_picture();
        pressure_release_draw_state();
        change_state_to(151, 1000);
      }
      press_any_button_to_start_game();
      break;
    case 151:
      // Box intro 2
      pressureReleaseIsTicking = 0;
      if (state.next == 0) {
        set_difficulty(1);
        display_intro_picture();
        change_state_to(GAME_BOX_STATE_START, 1000);
      }
      press_any_button_to_start_game();
      break;
    case 170:
      // Pressure Release game lost outro!
      if (state.next == 0) {
        pressureReleaseCurrentLevel = 0;
        pressureReleaseIsTicking = 0;
        clear_matrix();
        matrix_set_picture(matrixPicArrowDown, 0, 0, CRGB::Crimson);
        change_state_to(171, 2000);
      }
      break;
    case 171:
      // Game lost -> DONT show smiley -> new game or game over
      if (state.next == 0) {
        game_over_or_next_game(0);
      }
      break; 
    case 200:
      // Turn off every led and reset all games
      clear_matrix();
      change_state_to(GAME_BOX_STATE_START, 10);
      break;
  }
} /*}}} */

// Pressure Release Main loop
void pressure_release_loop() { /*{{{*/
  // No PR game?
  if (pressure_release_game_flag == 1) {
    return;
  }

  // If the PR should not tick, draw state and return
  if (pressureReleaseIsTicking == 0) {
    #ifdef DEBUG2
      if (pressureReleaseDebugTicker > 100) {
        Serial.println("pressure_release_loop: Not ticking!");
        pressureReleaseDebugTicker = 0;
      }
    #endif
    pressure_release_draw_state();
    return;
  }

  // Handle Button presses
  pressure_release_button_handling();

  // Change pressure level if the timer has elapsed
  if (millis() >= pressureReleaseTimerMsec + pressureReleaseNextStepMsec + pressureReleaseFirstStepMsec) {
    // First tick has a different msec offset and happens only once, so reset:
    pressureReleaseFirstStepMsec = 0;

    // Increase level in the direction
    // -1 : left
    //  1 : right
    pressureReleaseCurrentLevel += pressureReleaseCurrentDirection;

    #ifdef DEBUG2
      Serial.print("pressure_release_loop: pressureReleaseCurrentLevel = ");
      Serial.print(pressureReleaseCurrentLevel);
      Serial.print("; Direction =");
      Serial.print(pressureReleaseCurrentDirection);
    #endif

    //  0 --- 4 --- 8 (LEDs)
    // -4 --- 0 --- 4 (Level) 
    if (abs(pressureReleaseCurrentLevel) > (pressureReleaseNumLeds/2)) {
      // Lost!
      #ifdef DEBUG2
        Serial.println("");
        Serial.println("pressure_release_loop: Timer over!");
      #endif

      // Lost PR game:
      change_state_to(170, 1);
      return;
    } else if(pressureReleaseCurrentLevel == 0) {
      // When level is 0 choose a new random direction
      pressureReleaseCurrentDirection = random(0, 2) == 0 ? -1 : 1;

      #ifdef DEBUG2
        Serial.print("; New direction = ");
        Serial.print(pressureReleaseCurrentDirection);
      #endif
    } 
   
    // Draw state
    pressure_release_draw_state();

    #ifdef DEBUG2
      Serial.print("; Red LED index = ");
      Serial.println(pressureReleaseCurrentLevel + (pressureReleaseNumLeds/2));
    #endif

    // Set new timer
    pressureReleaseTimerMsec = millis();
  }
} /*}}} */

// Misc commands loop
void misc_commands_loop() { /*{{{*/
  // pressing all arcade buttons at the same time = reset
  uint8_t numberOfArcadeButtonsPressed = 0;
  for(uint8_t i = 0; i < gameArcadeButtonNumberOfButtons; i++) {
    if (digitalRead(gameArcadeButtonPinsDin[i]) == HIGH) {
      numberOfArcadeButtonsPressed++;
    }
  }

  // All pressed? Reset the box!
  if(numberOfArcadeButtonsPressed == 3) {
    #ifdef DEBUG2
      Serial.print("misc_commands_loop: Reset!");
    #endif
    change_state_to(200, 200);
    return;
  }

  // Pressing the outer arcade buttons + the right pressure release button adds +5 score
  // Works only in DEBUG or DEBUG2 mode
  #if defined (DEBUG) || defined (DEBUG2)
    if (digitalRead(gameArcadeButtonPinsDin[0]) == HIGH && 
        digitalRead(gameArcadeButtonPinsDin[gameArcadeButtonNumberOfButtons - 1]) == HIGH &&
        digitalRead(pressureReleasePinIn[1]) == HIGH)  {
      #ifdef DEBUG2
        Serial.print("misc_commands_loop: +5 score added!");
      #endif
      state.score = state.score + 5;
    }
  #endif
} /*}}} */

// Main arduino setup
void setup() /*{{{*/
{
  #if defined (DEBUG) || defined (DEBUG2)
    Serial.begin(9600);
  #endif
  
  matrix_setup();
  pressure_release_led_setup();
  pressure_release_setup();
  set_difficulty(0);
  game_setup();

  FastLED.setBrightness(matrixLedBrightness);
}/*}}}*/

// Main arduino loop
void loop() /*{{{*/
{
  // advance state
  update_state();

  // Reset all LEDs
  if (matrixLedsModified == 1) {
    fill_solid(matrixLeds, matrixNumLeds, CRGB::Black);
    matrixLedsModified = 0;
  }

  // misc commands loop
  misc_commands_loop();

  // pressure release loop
  pressure_release_loop();
 
  // Run master loop
  game_master_loop();

  // If MATRIX leds have been modified, show them
  if (matrixLedsModified > 0) {
    update_matrix_leds();
  }

  // If PRESSURE RELEASE leds have been modified, show them
  if (pressureReleaseLedsModified > 0) {
    update_pressure_release_leds();
    pressureReleaseLedsModified = 0;
  }
} /*}}}*/
