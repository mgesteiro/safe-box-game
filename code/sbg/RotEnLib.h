/**
 * RotEnLib
 *
 * Minimalistic adhoc rotary encoder with button Arduino library,
 * using internal PULL-UP resistors and debouncing. No interrupts
 * are used.
 *
 * @file     RotEnLib.h
 * @author   mgesteiro
 * @date     20230115
 * @version  0.1.1
 * @copyright OpenSource, LICENSE GPLv3
 */
#ifndef ROTENLIB_H
#define ROTENLIB_H

#include <stdint.h>

class RELEncoder;
typedef void (*ButtonEventCallback)(RELEncoder&, uint32_t);

class RELEncoder
{
public:
  /**
   * Constructor.
   *
   * @param S1 pinA for the rotary encoder
   * @param S2 pinB for the rotary encoder
   * @param btn pin for the encoder's button
   * @param min minimum value for the output
   * @param max maximum value for the output
   * @param circular indicates if the values should be circular
   * @param threshold limit time to consider a "fast movement", in milliseconds
   * @param thdelta increment of output value per position change while in "fast movement"
   */
  RELEncoder(int S1, int S2, int btn, int min, int max, bool circular=false, int threshold=15, int thdelta=10);

  /**
   * Initialize pins.
   */
  void init();

  /**
   * Retrieve the current position/value of the encoder.
   */
  int getPosition();

  /**
   * Set the current position/value of the encoder.
   */
  void setPosition(int newpos);

  /**
   * Retrieve library button state (doesn't make any new read)
   */
  int getBtnState();

  /**
   * Set onPress callback function.
   */
  void setButtonOnPressCB(ButtonEventCallback);

  /**
   * Set onRelease callback function.
   */
  void setButtonOnReleaseCB(ButtonEventCallback);

  /**
   * Set onLongPress callback function.
   */
  void setButtonOnLongPressCB(ButtonEventCallback);

  /**
   * Update function - call this function often to update the state.
   */
  void loop(void);

private:
  int _s1, _s2, _btn, _min, _max, _th, _thd;
  bool _circ;
  int _position;
  int _s1State, _s1LastState;
  int _btnState, _btnLastState;
  unsigned long _encsavedt, _btnsavedt, _btnpressed;
  bool _btndebouncing;
  ButtonEventCallback _callbackOnPress = 0;
  ButtonEventCallback _callbackOnRelease = 0;
  ButtonEventCallback _callbackOnLongPress = 0;
};

#endif
