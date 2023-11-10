#pragma once

#include <Arduino.h>

class ArduTimer {	
private:
	unsigned long _time;
	unsigned long _intervalMillis;
public:
	ArduTimer(unsigned long intervalMillis);
	boolean isWentOff(void);
	unsigned long getRemainingTimeMillis(void);
	unsigned long getIntervalMillis(void);
	void setIntervalMillis(unsigned long intervalMillis);
	void reset(void);
};
