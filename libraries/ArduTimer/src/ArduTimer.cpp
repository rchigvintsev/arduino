#include "ArduTimer.h"

#define ARDU_TIMER_ULONG_MAX_VALUE 4294967295UL

ArduTimer::ArduTimer(unsigned long intervalMillis) {
	setIntervalMillis(intervalMillis);
	reset();
}

boolean ArduTimer::isWentOff(void) {
	unsigned long now = millis();

	if (_intervalMillis == 0) {
		_time = now;
		return true;
	}

	boolean result = false;
	if (now - _time >= _intervalMillis) {
		do {
			_time += _intervalMillis;
			if (_time < _intervalMillis) {
				break;
			}
		} while (_time < now - _intervalMillis);
		result = true;
	}
	return result;
}

unsigned long ArduTimer::getRemainingTimeMillis(void) {
	if (_intervalMillis == 0) {
		return 0;
	}
	unsigned long now = millis();
	unsigned long delta;
	if (now < _time) {
		delta = ARDU_TIMER_ULONG_MAX_VALUE - _time + now;
	} else {
		delta = now - _time;
	}
	return delta > _intervalMillis ? 0 : _intervalMillis - delta;
}

unsigned long ArduTimer::getIntervalMillis(void) {
	return _intervalMillis;
}

void ArduTimer::setIntervalMillis(unsigned long intervalMillis) {
	if (_intervalMillis != intervalMillis) {
		_intervalMillis = intervalMillis;
		reset();
	}
}

void ArduTimer::reset(void) {
	_time = millis();
}
