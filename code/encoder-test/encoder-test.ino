
#include "RotEnLib.h"

RELEncoder encoder = RELEncoder(3, 2, 5, 0, 9); // pinA, pinB, btn, min, max
uint8_t savedposition = 0;

uint8_t code[] = {1, 2, 3, 4};

void updateDisplay(uint8_t code[], uint8_t digitpos)
{
  String text = "";
  for (uint8_t i = 0; i < 4; i++) text += code[i];
  Serial.println(text);
}

void setup()
{
  Serial.begin(9600);
  Serial.println("test encoder");
  // encoders button callbacks
  //encoder.setButtonOnReleaseCB(onButtonReleased);
  //encoder.setButtonOnLongPressCB(onButtonLongPressed);
  encoder.init();
  updateDisplay(code, 2);
}

void loop()
{
  // update controls status
  encoder.loop();

  // get encoder position
  uint8_t position = encoder.getPosition();

  // redraw sliders if neccessary
  if (position != savedposition)
  {
    savedposition = position;
    code[2] = position;
    updateDisplay(code, 2);
  }  
}
