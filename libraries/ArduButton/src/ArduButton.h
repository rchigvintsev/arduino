#pragma once

#include <Arduino.h>

#define ARDU_BUTTON_STATE_RELEASED 0
#define ARDU_BUTTON_STATE_PRESSED  1
#define ARDU_BUTTON_STATE_HELD     2

#define ARDU_BUTTON_DEFAULT_DEBOUNCE_TIMEOUT_MILLIS  50
#define ARDU_BUTTON_DEFAULT_HOLD_TIMEOUT_MILLIS     500

class ArduButton {
private:
	byte _pin;
	int _state;
	boolean _debounceState;
	unsigned long _debounceTime;
	byte _clickCounter;
	unsigned int _debounceTimeoutMillis;
	unsigned int _holdTimeoutMillis;
public:
	ArduButton(byte pin);
	void update(void);
	boolean isClicked(void);
	boolean isPressed(void);
	boolean isReleased(void);
	boolean isHeld(void);
	void setDebounceTimeoutMillis(unsigned int debounceTimeoutMillis);
	void setHoldTimeoutMillis(unsigned int holdTimeoutMillis);
	void reset(void);
};
