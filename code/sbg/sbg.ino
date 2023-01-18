/**
 * Safe BOX GAME firmware.
 *
 * @author mgesteiro
 * @version 1.0.0
 * @date 20230115
 * @copyright OpenSource, LICENSE GPLv3 and CCSABY40
 *
 * Libraries required: "Servo" and "Adafruit_SSD1306" (and dependencies)
 */

#define VERSION "1.0.0"

// CONFIGURATION
#define REL_PINA 2
#define REL_PINB 3
#define REL_BTN 5
#define SCREEN_ADDRESS 0x3C
#define SERVO_PIN 12
#define SERVO_UNLOCKED 110
#define SERVO_LOCKED 55
#define SHOW_RESULTS_IN_SCREEN false

#include "RotEnLib.h"  // rotary encoder library
#include <Adafruit_SSD1306.h>
#include "SBGlogo.h"  // startup screen
#include <Servo.h>

RELEncoder encoder = RELEncoder(REL_PINA, REL_PINB, REL_BTN, 0, 9, true); // pinA, pinB, btn, min, max, circular
Adafruit_SSD1306 display(128, 64, &Wire, -1);
Servo lockServo;

const uint8_t correctPositionPins[] = {7, 8, A2, A1};
const uint8_t correctNumberPins[]   = {6, 9, A3, A0};
uint8_t code[4], guess[4];
uint8_t currentDigit = 0;

uint32_t tactivity = 0;
uint8_t LEDstate = 0;

/**
 *******************************************************************************
 *   S E T U P   &   L O O P
 */

void setup()
{
  Serial.begin(9600);
  Serial.print("Safe BOX GAME v");
  Serial.println(VERSION);
  // LEDs
  initLEDs();
  // Encoder button callback
  encoder.setButtonOnPressCB(onButtonPressed);
  encoder.init();
  // Display
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
    Serial.println(F("SSD1306 allocation failed"));
  display.setTextColor(SSD1306_WHITE);
  // Servo
  lockServo.attach(SERVO_PIN);
  lockServo.write(SERVO_UNLOCKED);  // unlock the safe
  // Seed PRNG
  randomSeed(analogRead(A7));  // noise from A7 (not connected = floating)
  // Do intro
  doSplash();
  introLEDs();
  // Do start
  doStart();
}  // setup()

void loop()
{
  // avoids power-bank shutting down
  checkStandBy();

  // update encoder status
  encoder.loop();

  // get encoder position
  uint8_t position = encoder.getPosition();

  // redraw output if necessary
  if (position != guess[currentDigit])
  {
    guess[currentDigit] = position;
    updateDisplay(guess, currentDigit);
  }

  // the rest of the logic is done at the
  // encoder's onButtonPressed() callback
  // function where the "clicks" are processed

}  // loop()


/**
 *******************************************************************************
 *   S U P P O R T   F U N C T I O N S
 */

 void initLEDs()
{
  for (uint8_t i = 0; i < 4; i ++)
  {
    pinMode(correctPositionPins[i], OUTPUT);
    pinMode(correctNumberPins[i], OUTPUT);
  }
}

/**
 * Turn on/off each LED individually in bulk.
 *
 * @param state 1 bit per LED. Low nibble for "correct position" LEDs and
 *              high nibble for "correct number" LEDs.
 */
void turnLEDs(uint8_t state)
{
  LEDstate = state;  // save for standby check
  for (uint8_t i = 0; i < 4; i ++) digitalWrite(correctPositionPins[i], bitRead(state, i));
  for (uint8_t i = 4; i < 8; i ++) digitalWrite(correctNumberPins[i-4], bitRead(state, i));
}

/**
 * Show splash screen
 */
void doSplash()
{
  display.clearDisplay();
  display.drawBitmap(0, 0, logo_data, 128, 64, 1);  // put logo
  display.display();  // show logo
  delay(1000);
}

/**
 * LEDs animation.
 */
void introLEDs()
{
  // nice crack the code original animation
  uint8_t Anim[][2] =
  {
    {B00001111, 30},  // state, delay (in 10ms chunks)
    {B11110000, 30},
    {B00001111, 30},
    {B11110000, 30},
    {B11111111, 130},
  };
  for (uint8_t i = 0; i < sizeof(Anim)/2; i ++)
  {
    turnLEDs(Anim[i][0]);
    delay(Anim[i][1]*10);
  }
  turnLEDs(0);
}  // introLEDs()

/**
 * Generate a new code and clear current guess.
 */
void doCodes(bool show)
{
  for (uint8_t i = 0; i < 4; i ++)
  {
    code[i] = random(0,10);  // values from 0 to 9
    guess[i] = 0;
  } 
  // show code
  if (!show) return;
  Serial.print("New code: ");
  for (uint8_t i = 0; i < 4; i ++) Serial.print(code[i]);
  Serial.println();
}  // doCodes()

/**
 * Cycles through the "open", "wait for click", and "closed" states for
 * the lock. This is done this way because it is always what happens:
 * during the start and after a win.
 */
void doLock()
{
  // first: unlock and show message to lock
  lockServo.write(SERVO_UNLOCKED);  // unlock the safe
  display.clearDisplay();
  //display.invertDisplay(true);
  display.setTextSize(1);
  display.setCursor(30,18);
  display.print(F("pulsa para"));
  display.setTextSize(2);
  display.setCursor(30,30); 
  display.print(F("CERRAR"));
  display.display();
  // second: wait for user input
  waitForClick(0);
  // finally: lock and show the notification for 2 secs.
  lockServo.write(SERVO_LOCKED);  // lock the safe  
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(23,25);
  display.print(F("CERRADO"));
  display.display();
  delay(2000);
  //display.invertDisplay(false);
}  // doLock()

/**
 * Shows current guess-code and a cursor under the current digit being manipulated.
 *
 * @param guess guess code introduced by the user
 * @param digitpos position of the digit currently being updated
 */
void updateDisplay(uint8_t guess[], uint8_t digitpos)
{
  String text = "";
  for (uint8_t i = 0; i < 4; i++) text += guess[i];  // code[] to String
  display.setTextSize(4);
  display.clearDisplay();
  // default font size 5x7 (pixels) + 1 pixel separation between characters
  // (SCREEN_WIDTH - CHARACTER_WIDTH * PIXEL_MULTIPLIER * NUMBER_OF_DIGITS - (NUMBER_OF_DIGITS - 1) * PIXEL_MULTIPLIER) / 2
  // (128 - 5 * 4 * 4 - (4 - 1) * 4 ) / 2 = 18 <-- X
  // (SCREEN_HEIGHT - CHARACTER_HEIGHT * PIXEL_MULTIPLIER) / 2
  // ( 64 - 7 * 4 ) / 2 = 18 <-- Y
  display.setCursor(18, 16);  // 16 instead of 18 -> some room for under-cursor
  display.println(text);
  display.drawRect(18 + (digitpos * 24), 47, 20, 3, SSD1306_WHITE);  // under-cursor
  display.display();   
}  // updateDisplay()

/**
 * Shows the valid positions/numbers in the screen.
 *
 * @param validpos number of digits in valid position
 * @param validnum number of valid digits
 */
void showCheckResults(uint8_t validpos, uint8_t validnum)
{ 
  // LEDs
  // lo - positions, hi - numbers
  uint8_t state = B00000000;
  uint8_t mask = B00000001;
  for (uint8_t i = 0; i < validpos; i ++)
  {
    state |= mask;
    mask <<= 1;
  }
  mask = B00010000;
  for (uint8_t i = 0; i < validnum; i ++)
  {
    state |= mask;
    mask <<= 1;
  }
  turnLEDs(state);
  // Display
  display.clearDisplay();
  display.setTextSize(2);
  #if SHOW_RESULTS_IN_SCREEN
  // (128 - 6 * 5 * 2 - 5 * 2 ) / 2 = 29
  // (64 - 2 * 7 * 2 - 1 * 2 ) / 2  = 17
  String text = "NUM: ";
  text += validnum;
  display.setCursor(29, 17);
  display.print(text);
  text = "POS: ";
  text += validpos;
  display.setCursor(29, 33);
  display.print(text);
  #else
  display.setCursor(5, 25);
  display.print("INCORRECTO");
  #endif
  display.display();
}  // showCheckResults()

/**
 * Initialization routine: new code and reset parameters.
 */
void doStart()
{
  doLock();  // unlock, wait for click, lock
  doCodes(true);  // generate new code, clear guess
  currentDigit = 0;  // reset
  encoder.setPosition(guess[currentDigit]);  // reset
  updateDisplay(guess, currentDigit);  // show initial status
}

void doWin()
{
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(5, 25);
  display.print("CONSEGUIDO");
  display.display();
  lockServo.write(SERVO_UNLOCKED); // before intro
  introLEDs();
}

/**
 * Callback function to attend the encoder "OnPressed" event
 *
 * @param encoder which encoder sends the event
 * @param when moment in which occurred
 */
void onButtonPressed(RELEncoder& encoder, unsigned long when)
{
  currentDigit ++;  // change to next digit
  if (currentDigit > 3)  // was it the last digit?
  {
    // check provided code
    uint8_t copy[4]; // local copy of code for manipulation
    memcpy(copy, code, 4);
    // checking positions/numbers
    uint8_t validpos = 0, validnum = 0;
    for (uint8_t i = 0; i < 4; i++)
    {
      // valid position
      if (guess[i] == code[i]) validpos ++;
      // valid number
      for (uint8_t j = 0; j < 4; j++)
        if (guess[i] == copy[j]) {
          validnum ++;
          copy[j] = -1; // mark as used
          break; // no more checking
        }
    }
    // checking victory
    if (validpos == 4)
    {
      doWin();  // shows message
      doSplash();  // splash screen
      waitForClick(0);  // user must click
      doStart(); // start-up procedure
      return;
    }
    // show checking results
    showCheckResults(validpos, validnum);
    waitForClick(2000);  // 2 secs. maximum
    turnLEDs(0);
    // go to the beginning
    currentDigit = 0;
  }
  // update
  encoder.setPosition(guess[currentDigit]);
  updateDisplay(guess, currentDigit);

} // onButtonPressed()

/**
 * Checks that there is at least a minimum power consumption,
 * to avoid the power-bank shutting off.
 */
void checkStandBy()
{
  uint32_t tnow = millis();
  if (tnow - tactivity > 2000)
  {
    if (! LEDstate)
    {
      turnLEDs(B01000100 << random(0,2)  );  // turn-on a random pair of LEDs
      delay(5);  // for 5 milliseconds
      turnLEDs(0);
      tactivity = tnow; 
    }
  }
}

/**
 * Waits until the user clicks or the provided milliseconds pass.
 *
 * @param delay max time to wait, in milliseconds. Infinite if value is 0.
 */
void waitForClick(uint32_t tdelay)
{
  if (tdelay == 0) tdelay = 0xffffffff;
  delay(150);  // give time for button to be released
  uint32_t tnow = millis();
  while ( 
    (digitalRead(REL_BTN) == HIGH)
    && (millis() - tnow < tdelay)
  ) checkStandBy();
}