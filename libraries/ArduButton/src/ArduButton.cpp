#include "ArduButton.h"

ArduButton::ArduButton(byte pin) {
	_pin = pin;
	_debounceTimeoutMillis = ARDU_BUTTON_DEFAULT_DEBOUNCE_TIMEOUT_MILLIS;
	_holdTimeoutMillis = ARDU_BUTTON_DEFAULT_HOLD_TIMEOUT_MILLIS;
}

void ArduButton::update(void) {
	unsigned long now = millis();

	boolean state = !digitalRead(_pin);
	if (state && (_state != ARDU_BUTTON_STATE_PRESSED || _state != ARDU_BUTTON_STATE_HELD)) {
		if (!_debounceState) {
			_debounceState = true;
			_debounceTime = now;
			_clickCounter = 0;
		} else if (now - _debounceTime >= _debounceTimeoutMillis) {
			_state = ARDU_BUTTON_STATE_PRESSED;
		}
	}

	if (!state && _state != ARDU_BUTTON_STATE_RELEASED) {
		_debounceState = false;
		if (_state == ARDU_BUTTON_STATE_PRESSED) {
			_clickCounter = 1;
		}
		_state = ARDU_BUTTON_STATE_RELEASED;
	}

	if (_state == ARDU_BUTTON_STATE_PRESSED && now - _debounceTime >= _holdTimeoutMillis) {
		_state = ARDU_BUTTON_STATE_HELD;
	}
}

boolean ArduButton::isClicked(void) {
	if (_clickCounter == 1) {
		_clickCounter = 0;
		return true;
	}
	return false;
}

boolean ArduButton::isPressed(void) {
	return _state == ARDU_BUTTON_STATE_PRESSED;
}

boolean ArduButton::isReleased(void) {
  return _state == ARDU_BUTTON_STATE_RELEASED;
}

boolean ArduButton::isHeld(void) {
  return _state == ARDU_BUTTON_STATE_HELD;
}

void ArduButton::setDebounceTimeoutMillis(unsigned int debounceTimeoutMillis) {
	_debounceTimeoutMillis = debounceTimeoutMillis;
}

void ArduButton::setHoldTimeoutMillis(unsigned int holdTimeoutMillis) {
	_holdTimeoutMillis = holdTimeoutMillis;
}

void ArduButton::reset(void) {
	_state = ARDU_BUTTON_STATE_RELEASED;
	_debounceState = false;
	_debounceTime = 0;
	_clickCounter = 0;
}
