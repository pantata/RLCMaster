/*
 * ClickEncoder.h
 *
 *  Created on: 29. 3. 2015
 *      Author: ludek
 */

#ifndef CLICKENCODER_H_
#define CLICKENCODER_H_

#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#define HIGH 0x1
#define LOW  0x0

class ClickEncoder
{

public:
  typedef enum Button_e {
    Open = 0,
    Closed,

    Pressed,
    Held,
    Released,

    Clicked,
    DoubleClicked

  } Button;

public:
  ClickEncoder(uint8_t stepsPerNotch = 1);

  void service(void);
  int16_t getValue(void);

public:
  Button getButton(void);

public:
  void setDoubleClickEnabled(const bool &d)
  {
    doubleClickEnabled = d;
  }

  const bool getDoubleClickEnabled()
  {
    return doubleClickEnabled;
  }

public:
  void setAccelerationEnabled(const bool &a)
  {
    accelerationEnabled = a;
    if (accelerationEnabled == false) {
      acceleration = 0;
    }
  }

  const bool getAccelerationEnabled()
  {
    return accelerationEnabled;
  }

private:
   bool doubleClickEnabled;
  bool accelerationEnabled;
  uint8_t steps;
  volatile uint16_t acceleration;
  volatile int16_t delta;
  volatile int8_t last;
  volatile Button button;

};

#endif /* CLICKENCODER_H_ */
