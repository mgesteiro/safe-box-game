// RotEnLib
// --------
// author: mgesteiro
// date: 20211205

#include "RotEnLib.h"
#include "Arduino.h"

#define BTNDEBOUNCETIME 10
#define ENCDEBOUNCETIME 4
#define LONGPRESSTIME 800

// constructor
RELEncoder::RELEncoder(int S1, int S2, int btn, int min, int max, bool circular=false, int threshold=15, int thdelta=10)
{
  // Remember Hardware Setup
  _s1 = S1;
  _s2 = S2;
  _btn = btn;
  _min = min;
  _max = max;
  _circ = circular;
  _th = threshold;
  _thd = thdelta;
  
  // initial state
  _position = 0;
  _s1State = 0;
  _s1LastState = 0;
  _btnState = HIGH;
  _btnLastState = HIGH;
  _btndebouncing = false;
} // RELEncoder()

void RELEncoder::init()
{
  // Setup the input pins and turn on pullup resistor
  pinMode(_s1, INPUT_PULLUP);
  pinMode(_s2, INPUT_PULLUP);
  pinMode(_btn, INPUT_PULLUP);
}

int RELEncoder::getPosition()
{
  return _position;
} // getPosition()

void RELEncoder::setPosition(int newpos)
{
  _position = newpos;
} // setPosition()

int RELEncoder::getBtnState()
{
  return _btnState;
} // getBtnState()

void RELEncoder::setButtonOnPressCB(ButtonEventCallback callback)
{
  _callbackOnPress = callback;
} // setButtonOnPressCB()

void RELEncoder::setButtonOnReleaseCB(ButtonEventCallback callback)
{
  _callbackOnRelease = callback;
} // setButtonOnReleaseCB()

void RELEncoder::setButtonOnLongPressCB(ButtonEventCallback callback)
{
  _callbackOnLongPress = callback;
} // setButtonOnReleaseCB()

void RELEncoder::loop(void)
{
  unsigned long nowt = millis();
  // BUTTON
  int btnState = digitalRead(_btn);
  if (
      (btnState != _btnState) // button changed state
      && (! _btndebouncing) // not counting time yet
     ) 
  {
    // start counting (debounce time)
    _btnsavedt = nowt;
    _btndebouncing = true;
  }
  if (
      (_btndebouncing) // counting time 
      && ((nowt - _btnsavedt) > BTNDEBOUNCETIME) // bigger than debounce time
     ) 
  {
    // change of state
    if (_btnLastState != _btnState) _btnLastState = _btnState;
    _btnState = btnState;
    _btndebouncing = false;
    if ((_btnLastState == LOW) && (_btnState == HIGH)) if (_callbackOnRelease) _callbackOnRelease(*this, nowt-_btnpressed);
    if ((_btnLastState == HIGH) && (_btnState == LOW))
    {
      _btnpressed = nowt;
      if (_callbackOnPress) _callbackOnPress(*this, _btnpressed);
    }
  }
  if ((_btnLastState == HIGH) && (_btnState == LOW) && ((nowt-_btnpressed)>LONGPRESSTIME)) {
    //_btnLastState == LOW; //stop
    if (_callbackOnLongPress) _callbackOnLongPress(*this, nowt-_btnpressed);
    _btnpressed=nowt;
  }
  
  // ENCODER
  _s1State = digitalRead(_s1);
  // check if S1 has changed -> that means a pulse has occured
  if (_s1State != _s1LastState) {
    // simple debouncing filter
    if ((nowt-_encsavedt) < ENCDEBOUNCETIME) return;
    // only count when S1 is LOW
    if (! _s1State) {
      int delta = ((nowt-_encsavedt) < _th) ? _thd : 1;
      // if S2 state is the same as S1 state -> the encoder is rotating clockwise
      if (digitalRead(_s2) == _s1State) _position+=delta;
      else _position-=delta;

      if (_position > _max) _position = (_circ ? _min : _max);
      if (_position < _min) _position = (_circ ? _max : _min);

      _encsavedt=nowt; 
    }
  } 
  _s1LastState = _s1State;
} // loop()

// end
