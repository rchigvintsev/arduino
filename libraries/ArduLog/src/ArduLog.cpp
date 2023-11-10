#include "ArduLog.h"

ArduLogger::ArduLogger(String name, ArduLogLevel effectiveLevel) {
	_name = name;
	_effectiveLevel = effectiveLevel;
}

void ArduLogger::trace(String message) {
	if (!isOff() && _effectiveLevel == ArduLogLevel::TRACE) {
		log(message, ArduLogLevel::TRACE);
	}
}

void ArduLogger::debug(String message) {
	if (!isOff() && (_effectiveLevel == ArduLogLevel::TRACE || _effectiveLevel == ArduLogLevel::DEBUG)) {
		log(message, ArduLogLevel::DEBUG);
	}
}

void ArduLogger::info(String message) {
	if (!isOff() && (_effectiveLevel == ArduLogLevel::TRACE || _effectiveLevel == ArduLogLevel::DEBUG || _effectiveLevel == ArduLogLevel::INFO)) {
		log(message, ArduLogLevel::INFO);
	}
}

void ArduLogger::warn(String message) {
	if (!isOff() && (_effectiveLevel == ArduLogLevel::TRACE || _effectiveLevel == ArduLogLevel::DEBUG || _effectiveLevel == ArduLogLevel::INFO || _effectiveLevel == ArduLogLevel::WARN)) {
		log(message, ArduLogLevel::WARN);
	}
}

void ArduLogger::error(String message) {
	if (!isOff()) {
		log(message, ArduLogLevel::ERROR);
	}
}

boolean ArduLogger::isOff(void) {
	return _effectiveLevel == ArduLogLevel::OFF;
}

void ArduLogger::log(String message, ArduLogLevel level) {
	String resultMessage = "";
	
	unsigned long ms = millis();

	unsigned long hours = ms / ARDU_LOG_MILLIS_IN_HOUR;
	ms -= hours * ARDU_LOG_MILLIS_IN_HOUR;

	unsigned long minutes = ms / ARDU_LOG_MILLIS_IN_MINUTE;
	ms -= minutes * ARDU_LOG_MILLIS_IN_MINUTE;

	unsigned long seconds = ms / 1000;
	ms -= seconds * 1000;
	
	if (hours < 10) {
		resultMessage += "0";
	}
	resultMessage += String(hours) + ":";
	if (minutes < 10) {
		resultMessage += "0";
	}
	resultMessage += String(minutes) + ":";
	if (seconds < 10) {
		resultMessage += "0";
	}
	resultMessage += String(seconds) + ".";
	if (ms < 100) {
		resultMessage += "0";
		if (ms < 10) {
			resultMessage += "0";
		}
	}
	resultMessage += String(ms) + " ";
	
	switch (level) {
		case ArduLogLevel::TRACE:
			resultMessage += "TRACE ";
			break;
		case ArduLogLevel::DEBUG:
			resultMessage += "DEBUG ";
			break;
		case ArduLogLevel::INFO:
			resultMessage += "INFO ";
			break;
		case ArduLogLevel::WARN:
			resultMessage += "WARN ";
			break;
		case ArduLogLevel::ERROR:
			resultMessage += "ERROR ";
			break;
	}
	
	Serial.println(resultMessage + _name + " - " + message);
}
