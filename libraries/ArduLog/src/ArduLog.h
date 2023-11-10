#pragma once

#include <Arduino.h>

#define ARDU_LOG_MILLIS_IN_DAY 86400000
#define ARDU_LOG_MILLIS_IN_HOUR 3600000
#define ARDU_LOG_MILLIS_IN_MINUTE 60000

enum class ArduLogLevel {
	TRACE,
	DEBUG,
	INFO,
	WARN,
	ERROR,
	OFF
};

class ArduLogger {
private:
	String _name;
	ArduLogLevel _effectiveLevel;
	
	boolean isOff(void);
	void log(String message, ArduLogLevel level);
public:
	ArduLogger(String name, ArduLogLevel effectiveLevel = ArduLogLevel::DEBUG);
	void trace(String message);
	void debug(String message);
	void info(String message);
	void warn(String message);
	void error(String message);
};
