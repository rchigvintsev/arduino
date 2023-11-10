/*
 * Based on the code from Alex Gyver repository (https://github.com/AlexGyver/NixieClock_v2).
 */

#include <Wire.h>
#include <RTClib.h>
#include <EEPROM.h>

#define DUTY 180

#define FORCE_RTC_ADJUST false
#define INDICATOR_SWITCH_THRESHOLD 25
#define TIME_SYNC_TIMEOUT_MINUTES 15

#define NIGHT_MODE_ENABLED   true
#define NIGHT_STARTS_AT_HOUR   23
#define NIGHT_ENDS_AT_HOUR      7

#define INDICATOR_BRIGHTNESS          23
#define INDICATOR_BRIGHTNESS_AT_NIGHT  3

#define DOT_BRIGHTNESS            35
#define DOT_BRIGHTNESS_AT_NIGHT    0
#define DOT_TIMER_INTERVAL_MILLIS 20

#define BACKLIGHT_MODE_BREATHING      0
#define BACKLIGHT_MODE_MAX_BRIGHTNESS 1
#define BACKLIGHT_MODE_OFF            2

#define BACKLIGHT_MIN_BRIGHTNESS            20
#define BACKLIGHT_MAX_BRIGHTNESS           250
#define BACKLIGHT_MAX_BRIGHTNESS_AT_NIGHT   50
#define BACKLIGHT_BRIGHTNESS_STEP            2
#define BACKLIGHT_TIME_MILLIS             5000
#define BACKLIGHT_TURNED_OFF_TIME_MILLIS   400

#define GLITCHES_ALLOWED             true
#define GLITCH_MIN_INTERVAL_MILLIS  30000
#define GLITCH_MAX_INTERVAL_MILLIS 120000

#define BUTTON_DEBOUNCE_TIMEOUT_MILLIS  60
#define BUTTON_HOLD_TIMEOUT_MILLIS     500

#define BUTTON_STATE_RELEASED 0
#define BUTTON_STATE_PRESSED  1
#define BUTTON_STATE_HELD     2

#define CLOCK_STATE_NORMAL 0
#define CLOCK_STATE_SETUP  1

#define CLOCK_EFFECTS_NUMBER 6

#define CLOCK_EFFECT_NONE           0
#define CLOCK_EFFECT_FADING         1
#define CLOCK_EFFECT_FIGURE_REWIND  2
#define CLOCK_EFFECT_CATHODE_REWIND 3
#define CLOCK_EFFECT_TRAIN          4
#define CLOCK_EFFECT_RUBBER_BAND    5

#define ANTI_POISONING_INTERVAL_MINUTES 15
#define ANTI_POISONING_LOOPS_NUMBER      3
#define ANTI_POISONING_DELAY_MILLIS     10

#define INDICATOR1_PIN 6
#define INDICATOR2_PIN 5
#define INDICATOR3_PIN 4
#define INDICATOR4_PIN 3

#define DECODER0_PIN A0
#define DECODER1_PIN A1
#define DECODER2_PIN A2
#define DECODER3_PIN A3

#define BUTTON_SET_PIN    7
#define BUTTON_LEFT_PIN   8
#define BUTTON_RIGHT_PIN 12

#define DOT_PIN 10
#define BACKLIGHT_PIN 11
#define GENERATOR_PIN 9

#define EEPROM_KEY_FIRST_START      1023
#define EEPROM_KEY_CLOCK_EFFECT        0
#define EEPROM_KEY_BACKLIGHT_MODE      1
#define EEPROM_KEY_GLITCHES_ENABLED    2

const byte INDICATOR_PINS[] = {INDICATOR1_PIN, INDICATOR2_PIN, INDICATOR3_PIN, INDICATOR4_PIN};
const byte FIGURE_MASKS[] = {0b00001001, 0b00001000, 0b00000000, 0b00000101, 0b00000100, 0b00000111, 0b00000011, 0b00000110, 0b00000010, 0b00000001};
const byte CATHODE_ORDER[] = {1, 0, 2, 9, 3, 8, 4, 7, 5, 6};

volatile byte indicatorMaxBrightness;
int indicatorBrightnessCounter;
boolean indicatorBrightnessRaising;
volatile byte indicatorStates[4] = {HIGH, HIGH, HIGH, HIGH};
volatile byte indicatorBrightnessCounters[4];
volatile byte indicatorDimmingThresholds[4];
volatile byte currentIndicator;

byte hours, minutes, seconds;
byte userHours, userMinutes;

volatile byte figures[4];
byte nextFigures[4];
boolean changedFigures[4];

boolean halfSecondPassed;
boolean clockUpdateRequired;
byte timeToSyncMinutes = TIME_SYNC_TIMEOUT_MINUTES;

boolean dotTurnedOn;
boolean dotBrightnessRaising;
byte dotBrightnessStep;
byte dotMaxBrightness;
int dotBrightnessCounter;

byte backlightMode;
boolean backlightTurnedOn;
boolean backlightBrightnessRaising = true;
byte backlightMaxBrightness;
int backlightBrightnessCounter;
unsigned long backlightTimerInterval;

#if GLITCHES_ALLOWED
boolean glitchesEnabled = true;
boolean glitching;
byte glitchingIndicatorIndex;
boolean glitchingIndicatorEnabled;
byte glitchCounter;
byte glitchCounterThreshold;
unsigned long glitchTimerInterval;
#endif

int buttonStates[3];
boolean buttonDebounceStates[3];
unsigned long buttonDebounceTimes[3];
byte buttonClickCounters[3];

volatile byte clockState = CLOCK_STATE_NORMAL;
byte nextClockState = CLOCK_STATE_SETUP;
volatile boolean adjustingHours = true;

byte clockEffect = CLOCK_EFFECT_NONE;
const unsigned long CLOCK_EFFECT_TIMER_INTERVALS[] = {0, 130, 50, 40, 80, 80};
boolean clockEffectInitialized;
byte startingCathodes[4];
byte endingCathodes[4];
byte trainCounter;
boolean trainLeaving;
byte rubberBandCounter;

RTC_DS3231 rtc;

unsigned long clockTimer = millis();
unsigned long clockEffectTimer = millis();
unsigned long dotTimer = millis();
unsigned long backlightTimer = millis();
#if GLITCHES_ALLOWED
unsigned long glitchTimer = millis();
#endif
unsigned long setupTimer = millis();

const byte CRT_GAMMA[256] PROGMEM = {
  0,    0,    1,    1,    1,    1,    1,    1,
  1,    1,    1,    1,    1,    1,    1,    1,
  2,    2,    2,    2,    2,    2,    2,    2,
  3,    3,    3,    3,    3,    3,    4,    4,
  4,    4,    4,    5,    5,    5,    5,    6,
  6,    6,    7,    7,    7,    8,    8,    8,
  9,    9,    9,    10,   10,   10,   11,   11,
  12,   12,   12,   13,   13,   14,   14,   15,
  15,   16,   16,   17,   17,   18,   18,   19,
  19,   20,   20,   21,   22,   22,   23,   23,
  24,   25,   25,   26,   26,   27,   28,   28,
  29,   30,   30,   31,   32,   33,   33,   34,
  35,   35,   36,   37,   38,   39,   39,   40,
  41,   42,   43,   43,   44,   45,   46,   47,
  48,   49,   49,   50,   51,   52,   53,   54,
  55,   56,   57,   58,   59,   60,   61,   62,
  63,   64,   65,   66,   67,   68,   69,   70,
  71,   72,   73,   74,   75,   76,   77,   79,
  80,   81,   82,   83,   84,   85,   87,   88,
  89,   90,   91,   93,   94,   95,   96,   98,
  99,   100,  101,  103,  104,  105,  107,  108,
  109,  110,  112,  113,  115,  116,  117,  119,
  120,  121,  123,  124,  126,  127,  129,  130,
  131,  133,  134,  136,  137,  139,  140,  142,
  143,  145,  146,  148,  149,  151,  153,  154,
  156,  157,  159,  161,  162,  164,  165,  167,
  169,  170,  172,  174,  175,  177,  179,  180,
  182,  184,  186,  187,  189,  191,  193,  194,
  196,  198,  200,  202,  203,  205,  207,  209,
  211,  213,  214,  216,  218,  220,  222,  224,
  226,  228,  230,  232,  233,  235,  237,  239,
  241,  243,  245,  247,  249,  251,  253,  255,
};

void setup() {
  Serial.begin(9600);

  randomSeed(analogRead(6) + analogRead(7));

  pinMode(INDICATOR1_PIN, OUTPUT);
  pinMode(INDICATOR2_PIN, OUTPUT);
  pinMode(INDICATOR3_PIN, OUTPUT);
  pinMode(INDICATOR4_PIN, OUTPUT);

  pinMode(DECODER0_PIN, OUTPUT);
  pinMode(DECODER1_PIN, OUTPUT);
  pinMode(DECODER2_PIN, OUTPUT);
  pinMode(DECODER3_PIN, OUTPUT);

  pinMode(DOT_PIN, OUTPUT);
  pinMode(BACKLIGHT_PIN, OUTPUT);
  pinMode(GENERATOR_PIN, OUTPUT);

  pinMode(BUTTON_SET_PIN, INPUT_PULLUP);
  pinMode(BUTTON_LEFT_PIN, INPUT_PULLUP);
  pinMode(BUTTON_RIGHT_PIN, INPUT_PULLUP);

  // Change PWM frequency to 31.4 KHz for Timer 1 (D9/D10 pins)
  TCCR1B = 0b00000001;

  analogWrite(GENERATOR_PIN, DUTY);

  // Change PWM frequency to 7.8 KHz for Timer 2 (D3/D11 pins)
  TCCR2B = 0b00000010;

  TCCR2A |= (1 << WGM21);
  TIMSK2 |= (1 << OCIE2A);

  rtc.begin();
  if (rtc.lostPower() || FORCE_RTC_ADJUST) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  DateTime now = rtc.now();
  hours = now.hour();
  minutes = now.minute();
  seconds = now.second();

  if (EEPROM.read(EEPROM_KEY_FIRST_START) != 100) {
    EEPROM.put(EEPROM_KEY_FIRST_START, 100);
    EEPROM.put(EEPROM_KEY_CLOCK_EFFECT, clockEffect);
    EEPROM.put(EEPROM_KEY_BACKLIGHT_MODE, backlightMode);
    #if GLITCHES_ALLOWED
    EEPROM.put(EEPROM_KEY_GLITCHES_ENABLED, glitchesEnabled);
    #endif
  }
  EEPROM.get(EEPROM_KEY_CLOCK_EFFECT, clockEffect);
  EEPROM.get(EEPROM_KEY_BACKLIGHT_MODE, backlightMode);
  #if GLITCHES_ALLOWED
  EEPROM.get(EEPROM_KEY_GLITCHES_ENABLED, glitchesEnabled);
  #endif

  showTime(hours, minutes);
  changeBrightness();

  #if GLITCHES_ALLOWED
  glitchTimerInterval = random(GLITCH_MIN_INTERVAL_MILLIS, GLITCH_MAX_INTERVAL_MILLIS);
  #endif
}

void loop() {
  updateTime();
  updateClock();
  updateDot();
  updateBacklight();
  updateGlitches();
  updateButtons();
  updateSetup();
}

void updateTime() {
  if (isTimeUpdateRequired()) {
    halfSecondPassed = !halfSecondPassed;
    if (!halfSecondPassed) {
      // Whole second passed since last increment
      dotTurnedOn = true;

      seconds++;
      if (seconds > 59) {
        seconds = 0;
        minutes++;

        clockUpdateRequired = true;

        if (--timeToSyncMinutes == 0) {
          syncTime();
        }

        if (minutes % ANTI_POISONING_INTERVAL_MINUTES == 0) {
          runAntiPoisoning();
        }
      }

      if (minutes > 59) {
        minutes = 0;
        hours++;

        if (hours > 23) {
          hours = 0;
        }

        changeBrightness();
      }
    }
  }
}

void updateClock() {
  if (clockUpdateRequired && clockState == CLOCK_STATE_NORMAL) {
    if (clockEffect == CLOCK_EFFECT_NONE) {
      updateClockEffectNone();
    } else {
      if (clockEffect == CLOCK_EFFECT_FADING) {
        updateClockEffectFading();
      } else if (clockEffect == CLOCK_EFFECT_FIGURE_REWIND) {
        updateClockEffectFigureRewind();
      } else if (clockEffect == CLOCK_EFFECT_CATHODE_REWIND) {
        updateClockEffectCathodeRewind();
      } else if (clockEffect == CLOCK_EFFECT_TRAIN) {
        updateClockEffectTrain();
      } else if (clockEffect == CLOCK_EFFECT_RUBBER_BAND) {
        updateClockEffectRubberBand();
      }
    }
  }
}

void updateClockEffectNone() {
  showTime(hours, minutes);
  clockUpdateRequired = false;
}

void updateClockEffectFading() {
  if (!clockEffectInitialized) {
    showNextTime(hours, minutes);
    for (byte i = 0; i < 4; i++) {
      changedFigures[i] = figures[i] != nextFigures[i];
    }

    clockEffectInitialized = true;
  }

  if (isClockEffectUpdateRequired()) {
    if (!indicatorBrightnessRaising) {
      indicatorBrightnessCounter--;
      if (indicatorBrightnessCounter <= 0) {
        indicatorBrightnessRaising = true;
        indicatorBrightnessCounter = 0;
        showTime(hours, minutes);
      }
    } else {
      indicatorBrightnessCounter++;
      if (indicatorBrightnessCounter >= indicatorMaxBrightness) {
        indicatorBrightnessRaising = false;
        indicatorBrightnessCounter = indicatorMaxBrightness;
        clockEffectInitialized = false;
        clockUpdateRequired = false;
      }
    }

    for (byte i = 0; i < 4; i++) {
      if (changedFigures[i]) {
        indicatorDimmingThresholds[i] = indicatorBrightnessCounter;
      }
    }
  }
}

void updateClockEffectFigureRewind() {
  if (!clockEffectInitialized) {
    showNextTime(hours, minutes);
    for (byte i = 0; i < 4; i++) {
      changedFigures[i] = figures[i] != nextFigures[i];
    }

    clockEffectInitialized = true;
  }

  if (isClockEffectUpdateRequired()) {
    byte unchangedFigureCounter = 0;
    for (byte i = 0; i < 4; i++) {
      if (changedFigures[i]) {
        if (figures[i] == 0) {
          figures[i] = 9;
        } else {
          figures[i]--;
        }
        if (figures[i] == nextFigures[i]) {
          changedFigures[i] = false;
        }
      } else {
        unchangedFigureCounter++;
      }
    }

    if (unchangedFigureCounter == 4) {
      clockEffectInitialized = false;
      clockUpdateRequired = false;
    }
  }
}

void updateClockEffectCathodeRewind() {
  if (!clockEffectInitialized) {
    showNextTime(hours, minutes);
    for (byte i = 0; i < 4; i++) {
      changedFigures[i] = figures[i] != nextFigures[i];
      if (changedFigures[i]) {
        for (byte j = 0; j < 10; j++) {
          if (CATHODE_ORDER[j] == figures[i]) {
            startingCathodes[i] = j;
          }
          if (CATHODE_ORDER[j] == nextFigures[i]) {
            endingCathodes[i] = j;
          }
        }
      }
    }

    clockEffectInitialized = true;
  }

  if (isClockEffectUpdateRequired()) {
    byte unchangedFigureCounter = 0;
    for (byte i = 0; i < 4; i++) {
      if (changedFigures[i]) {
        if (startingCathodes[i] > endingCathodes[i]) {
          startingCathodes[i]--;
          figures[i] = CATHODE_ORDER[startingCathodes[i]];
        } else if (startingCathodes[i] < endingCathodes[i]) {
          startingCathodes[i]++;
          figures[i] = CATHODE_ORDER[startingCathodes[i]];
        } else {
          changedFigures[i] = false;
        }
      } else {
        unchangedFigureCounter++;
      }
    }

    if (unchangedFigureCounter == 4) {
      clockEffectInitialized = false;
      clockUpdateRequired = false;
    }
  }
}

void updateClockEffectTrain() {
  if (!clockEffectInitialized) {
    showNextTime(hours, minutes);
    trainCounter = 0;
    trainLeaving = true;
    clockEffectTimer = millis();
    clockEffectInitialized = true;
  }

  if (isClockEffectUpdateRequired()) {
    if (trainLeaving) {
      for (byte i = 3; i > trainCounter; i--) {
        figures[i] = figures[i - 1];
      }
      indicatorStates[trainCounter++] = LOW;
      if (trainCounter == 4) {
        trainLeaving = false;
        trainCounter = 0;
      }
    } else {
      for (byte i = trainCounter; i > 0; i--) {
        figures[i] = figures[i - 1];
      }
      figures[0] = nextFigures[3 - trainCounter];
      indicatorStates[trainCounter] = HIGH;
      trainCounter++;
      if (trainCounter == 4) {
        clockEffectInitialized = false;
        clockUpdateRequired = false;
      }
    }
  }
}

void updateClockEffectRubberBand() {
  if (!clockEffectInitialized) {
    showNextTime(hours, minutes);
    rubberBandCounter = 0;
    clockEffectTimer = millis();
    clockEffectInitialized = true;
  }

  if (isClockEffectUpdateRequired()) {
    switch (rubberBandCounter++) {
      case 1:
        indicatorStates[3] = LOW;
        break;
      case 2:
        indicatorStates[2] = LOW;
        figures[3] = figures[2];
        indicatorStates[3] = HIGH;
        break;
      case 3:
        indicatorStates[3] = LOW;
        break;
      case 4:
        indicatorStates[1] = LOW;
        figures[2] = figures[1];
        indicatorStates[2] = HIGH;
        break;
      case 5:
        indicatorStates[2] = LOW;
        figures[3] = figures[1];
        indicatorStates[3] = HIGH;
        break;
      case 6:
        indicatorStates[3] = LOW;
        break;
      case 7:
        indicatorStates[0] = LOW;
        figures[1] = figures[0];
        indicatorStates[1] = HIGH;
        break;
      case 8:
        indicatorStates[1] = LOW;
        figures[2] = figures[0];
        indicatorStates[2] = HIGH;
        break;
      case 9:
        indicatorStates[2] = LOW;
        figures[3] = figures[0];
        indicatorStates[3] = HIGH;
        break;
      case 10:
        indicatorStates[3] = LOW;
        break;
      case 11:
        figures[0] = nextFigures[3];
        indicatorStates[0] = HIGH;
        break;
      case 12:
        indicatorStates[0] = LOW;
        figures[1] = nextFigures[3];
        indicatorStates[1] = HIGH;
        break;
      case 13:
        indicatorStates[1] = LOW;
        figures[2] = nextFigures[3];
        indicatorStates[2] = HIGH;
        break;
      case 14:
        indicatorStates[2] = LOW;
        figures[3] = nextFigures[3];
        indicatorStates[3] = HIGH;
        break;
      case 15:
        figures[0] = nextFigures[2];
        indicatorStates[0] = HIGH;
        break;
      case 16:
        indicatorStates[0] = LOW;
        figures[1] = nextFigures[2];
        indicatorStates[1] = HIGH;
        break;
      case 17:
        indicatorStates[1] = LOW;
        figures[2] = nextFigures[2];
        indicatorStates[2] = HIGH;
        break;
      case 18:
        figures[0] = nextFigures[1];
        indicatorStates[0] = HIGH;
        break;
      case 19:
        indicatorStates[0] = LOW;
        figures[1] = nextFigures[1];
        indicatorStates[1] = HIGH;
        break;
      case 20:
        figures[0] = nextFigures[0];
        indicatorStates[0] = HIGH;
        break;
      case 21:
        clockEffectInitialized = false;
        clockUpdateRequired = false;
        break;
    }
  }
}

void updateDot() {
  if (dotTurnedOn && isDotUpdateRequired()) {
    if (clockState == CLOCK_STATE_SETUP) {
      dotBrightnessCounter = dotMaxBrightness;
    } else {
      if (dotBrightnessRaising) {
        dotBrightnessCounter += dotBrightnessStep;
        if (dotBrightnessCounter >= dotMaxBrightness) {
          dotBrightnessRaising = false;
          dotBrightnessCounter = dotMaxBrightness;
        }
      } else {
        dotBrightnessCounter -= dotBrightnessStep;
        if (dotBrightnessCounter <= 0) {
          dotTurnedOn = false;
          dotBrightnessRaising = true;
          dotBrightnessCounter = 0;
        }
      }
    }
    digitalWrite(DOT_PIN, getCrtPwm(dotBrightnessCounter));
  }
}

void updateBacklight() {
  if (backlightMode == BACKLIGHT_MODE_BREATHING && isBacklightUpdateRequired()) {
    if (backlightMaxBrightness > 0) {
      if (backlightBrightnessRaising) {
        if (!backlightTurnedOn) {
          backlightTurnedOn = true;
          backlightTimerInterval = (float) BACKLIGHT_BRIGHTNESS_STEP / backlightMaxBrightness / 2 * BACKLIGHT_TIME_MILLIS;
        }

        backlightBrightnessCounter += BACKLIGHT_BRIGHTNESS_STEP;

        if (backlightBrightnessCounter >= backlightMaxBrightness) {
          backlightBrightnessRaising = false;
          backlightBrightnessCounter = backlightMaxBrightness;
        }
      } else {
        backlightBrightnessCounter -= BACKLIGHT_BRIGHTNESS_STEP;

        if (backlightBrightnessCounter <= BACKLIGHT_MIN_BRIGHTNESS) {
          backlightTurnedOn = false;
          backlightBrightnessRaising = true;
          backlightBrightnessCounter = BACKLIGHT_MIN_BRIGHTNESS;
          backlightTimerInterval = BACKLIGHT_TURNED_OFF_TIME_MILLIS;
        }
      }

      analogWrite(BACKLIGHT_PIN, getCrtPwm(backlightBrightnessCounter));
    } else {
      digitalWrite(BACKLIGHT_PIN, 0);
    }
  }
}

void updateGlitches() {
#if GLITCHES_ALLOWED
  if (clockState == CLOCK_STATE_NORMAL && glitchesEnabled && isGlitchesUpdateRequired()) {
    if (!glitching) {
      if (seconds > 5 && seconds < 55) {
        glitching = true;
        glitchingIndicatorEnabled = false;
        glitchCounter = 0;
        glitchCounterThreshold = random(2, 6);
        glitchingIndicatorIndex = random(0, 4);
        glitchTimerInterval = random(1, 6) * 20;
      }
    } else {
      if (++glitchCounter > glitchCounterThreshold) {
        glitching = false;
        indicatorDimmingThresholds[glitchingIndicatorIndex] = indicatorMaxBrightness;
        glitchTimerInterval = random(GLITCH_MIN_INTERVAL_MILLIS, GLITCH_MAX_INTERVAL_MILLIS);
      } else {
        indicatorDimmingThresholds[glitchingIndicatorIndex] = glitchingIndicatorEnabled ? indicatorMaxBrightness : 0;
        glitchingIndicatorEnabled = !glitchingIndicatorEnabled;
        glitchTimerInterval = random(1, 6) * 20;
      }
    }
  }
#endif
}

void updateButtons() {
  updateButton(BUTTON_SET_PIN);
  updateButton(BUTTON_LEFT_PIN);
  updateButton(BUTTON_RIGHT_PIN);

  if (isButtonHeld(BUTTON_SET_PIN)) {
    if (nextClockState == CLOCK_STATE_SETUP) {
      if (clockState != CLOCK_STATE_SETUP) {
        clockState = CLOCK_STATE_SETUP;
        adjustingHours = true;
        userHours = hours;
        userMinutes = minutes;
      }
    } else if (clockState != CLOCK_STATE_NORMAL) {
      hours = userHours;
      minutes = userMinutes;
      seconds = 0;

      DateTime now = rtc.now();
      rtc.adjust(DateTime(now.year(), now.month(), now.day(), hours, minutes, 0));

      for (byte i = 0; i < 4; i++) {
        indicatorStates[i] = HIGH;
      }
      changeBrightness();
      clockState = CLOCK_STATE_NORMAL;
    }
  } else if (isButtonReleased(BUTTON_SET_PIN)) {
    if (clockState == CLOCK_STATE_SETUP) {
      nextClockState = CLOCK_STATE_NORMAL;

      if (isButtonClicked(BUTTON_SET_PIN)) {
        adjustingHours = !adjustingHours;
      }
    
      if (isButtonClicked(BUTTON_RIGHT_PIN)) {
        if (adjustingHours) {
          userHours++;
          if (userHours > 23) {
            userHours = 0;
          }
        } else {
          userMinutes++;
          if (userMinutes > 59) {
            userMinutes = 0;
            userHours++;
            if (userHours > 23) {
              userHours = 0;
            }
          }
        }
      }

      if (isButtonClicked(BUTTON_LEFT_PIN)) {
        if (adjustingHours) {
          if (userHours == 0) {
            userHours = 23;
          } else {
            userHours--;
          }
        } else {
          if (userMinutes == 0) {
            userMinutes = 59;

            if (userHours == 0) {
              userHours = 23;
            } else {
              userHours--;
            }
          } else {
            userMinutes--;
          }
        }
      }
    } else {
      nextClockState = CLOCK_STATE_SETUP;

      if (isButtonClicked(BUTTON_RIGHT_PIN)) {
        if (++clockEffect == CLOCK_EFFECTS_NUMBER) {
          clockEffect = 0;
        }
        EEPROM.put(EEPROM_KEY_CLOCK_EFFECT, clockEffect);

        for (byte i = 0; i < 4; i++) {
          indicatorStates[i] = HIGH;
          indicatorDimmingThresholds[i] = indicatorMaxBrightness;
        }
        indicatorBrightnessRaising = false;
        indicatorBrightnessCounter = indicatorMaxBrightness;

        for (byte i = 0; i < 4; i++) {
          figures[i] = clockEffect;
        }

        clockEffectInitialized = false;
        clockUpdateRequired = true;
      }

      if (isButtonClicked(BUTTON_LEFT_PIN)) {
        if (++backlightMode == 3) {
          backlightMode = 0;
        }
        EEPROM.put(EEPROM_KEY_BACKLIGHT_MODE, backlightMode);
        if (backlightMode == BACKLIGHT_MODE_MAX_BRIGHTNESS) {
          analogWrite(BACKLIGHT_PIN, backlightMaxBrightness);
        } else if (backlightMode == BACKLIGHT_MODE_OFF) {
          digitalWrite(BACKLIGHT_PIN, LOW);
        }
      }

      #if GLITCHES_ALLOWED
      if (isButtonHeld(BUTTON_LEFT_PIN)) {
        glitchesEnabled = !glitchesEnabled;
        EEPROM.put(EEPROM_KEY_GLITCHES_ENABLED, glitchesEnabled);
      }
      #endif
    }
  }
}

void updateButton(int button) {
  unsigned long now = millis();

  boolean state = !digitalRead(button);
  byte btnIndex = getButtonIndex(button);

  if (state && (buttonStates[btnIndex] != BUTTON_STATE_PRESSED || buttonStates[btnIndex] != BUTTON_STATE_HELD)) {
    if (!buttonDebounceStates[btnIndex]) {
      buttonDebounceStates[btnIndex] = true;
      buttonDebounceTimes[btnIndex] = now;
      buttonClickCounters[btnIndex] = 0;
    } else if (now - buttonDebounceTimes[btnIndex] >= BUTTON_DEBOUNCE_TIMEOUT_MILLIS) {
      buttonStates[btnIndex] = BUTTON_STATE_PRESSED;
    }
  }

  if (!state && buttonStates[btnIndex] != BUTTON_STATE_RELEASED) {
      buttonDebounceStates[btnIndex] = false;
      if (buttonStates[btnIndex] == BUTTON_STATE_PRESSED) {
        buttonClickCounters[btnIndex] = 1;
      }
      buttonStates[btnIndex] = BUTTON_STATE_RELEASED;
  }

  if (buttonStates[btnIndex] == BUTTON_STATE_PRESSED && now - buttonDebounceTimes[btnIndex] >= BUTTON_HOLD_TIMEOUT_MILLIS) {
    buttonStates[btnIndex] = BUTTON_STATE_HELD;
  }
}

void updateSetup() {
  if (clockState == CLOCK_STATE_SETUP && isSetupUpdateRequired()) {
    showTime(userHours, userMinutes);
    if (adjustingHours) {
      indicatorStates[0] = indicatorStates[1] = HIGH;
      indicatorStates[2] = indicatorStates[3] = LOW;
    } else {
      indicatorStates[0] = indicatorStates[1] = LOW;
      indicatorStates[2] = indicatorStates[3] = HIGH;
    }
  }
}

boolean isButtonClicked(int button) {
  byte btnIndex = getButtonIndex(button);
  if (buttonClickCounters[btnIndex] == 1) {
    buttonClickCounters[btnIndex] = 0;
    return true;
  }
  return false;
}

boolean isButtonHeld(int button) {
  return buttonStates[getButtonIndex(button)] == BUTTON_STATE_HELD;
}

boolean isButtonReleased(int button) {
  return buttonStates[getButtonIndex(button)] == BUTTON_STATE_RELEASED;
}

byte getButtonIndex(int button) {
  return button == BUTTON_SET_PIN ? 0 : (button == BUTTON_LEFT_PIN ? 1 : 2);
}

boolean isTimeUpdateRequired() {
  return isTimerWentOff(clockTimer, 500);
}

boolean isClockEffectUpdateRequired() {
  return isTimerWentOff(clockEffectTimer, CLOCK_EFFECT_TIMER_INTERVALS[clockEffect]);
}

boolean isDotUpdateRequired() {
  return isTimerWentOff(dotTimer, DOT_TIMER_INTERVAL_MILLIS);
}

boolean isBacklightUpdateRequired() {
  return isTimerWentOff(backlightTimer, backlightTimerInterval);
}

#if GLITCHES_ALLOWED
boolean isGlitchesUpdateRequired() {
  return isTimerWentOff(glitchTimer, glitchTimerInterval);
}
#endif

boolean isSetupUpdateRequired() {
  return isTimerWentOff(setupTimer, 100);
}

boolean isTimerWentOff(unsigned long &timer, unsigned long interval) {
  unsigned long now = millis();

  if (interval == 0) {
    timer = now;
    return true;
  }

  boolean result = false;
  if (now - timer >= interval) {
    do {
      timer += interval;
      if (timer < interval) {
        break;
      }
    } while (timer < now - interval);
    result = true;
  }
  return result;
}

void showTime(byte hours, byte minutes) {
  figures[0] = (byte) hours / 10;
  figures[1] = (byte) hours % 10;

  figures[2] = (byte) minutes / 10;
  figures[3] = (byte) minutes % 10;
}

void showNextTime(byte hours, byte minutes) {
  nextFigures[0] = (byte) hours / 10;
  nextFigures[1] = (byte) hours % 10;

  nextFigures[2] = (byte) minutes / 10;
  nextFigures[3] = (byte) minutes % 10;
}

void syncTime() {
  DateTime now = rtc.now();
  hours = now.hour();
  minutes = now.minute();
  seconds = now.second();

  timeToSyncMinutes = TIME_SYNC_TIMEOUT_MINUTES;
}

void changeBrightness() {
  indicatorMaxBrightness = INDICATOR_BRIGHTNESS;
  dotMaxBrightness = DOT_BRIGHTNESS;
  backlightMaxBrightness = BACKLIGHT_MAX_BRIGHTNESS;

#if NIGHT_MODE_ENABLED
  if ((hours >= NIGHT_STARTS_AT_HOUR && hours <= 23) || (hours >= 0 && hours < NIGHT_ENDS_AT_HOUR)) {
    indicatorMaxBrightness = INDICATOR_BRIGHTNESS_AT_NIGHT;
    dotMaxBrightness = DOT_BRIGHTNESS_AT_NIGHT;
    backlightMaxBrightness = BACKLIGHT_MAX_BRIGHTNESS_AT_NIGHT;
  }
#endif

  for (byte i = 0; i < 4; i++) {
    indicatorDimmingThresholds[i] = indicatorMaxBrightness;
  }
  indicatorBrightnessCounter = indicatorMaxBrightness;

  dotBrightnessStep = ceil((float) dotMaxBrightness * 2 / 500 * DOT_TIMER_INTERVAL_MILLIS);
  if (dotBrightnessStep == 0) {
    dotBrightnessStep = 1;
  }

  if (backlightMaxBrightness > 0) {
    backlightTimerInterval = (float) BACKLIGHT_BRIGHTNESS_STEP / backlightMaxBrightness / 2 * BACKLIGHT_TIME_MILLIS;
  }
  if (backlightMode == BACKLIGHT_MODE_MAX_BRIGHTNESS) {
    analogWrite(BACKLIGHT_PIN, backlightMaxBrightness);
  }
}

byte getCrtPwm(byte value) {
  return pgm_read_byte(&(CRT_GAMMA[value]));
}

byte getIndicatorDimmingThreshold(byte indicator) {
  if (clockState == CLOCK_STATE_SETUP && ((adjustingHours && indicator < 2) || (!adjustingHours && indicator > 1))) {
    return indicatorMaxBrightness;
  }
  return indicatorDimmingThresholds[indicator];
}

void runAntiPoisoning() {
  for (byte i = 0; i < ANTI_POISONING_LOOPS_NUMBER; i++) {
    for (byte j = 0; j < 10; j++) {
      for (byte k = 0; k < 4; k++) {
        if (figures[k] == 0) {
          figures[k] = 9;
        } else {
          figures[k]--;
        }
      }
      delay(ANTI_POISONING_DELAY_MILLIS);
    }
  }

}

// Main function that shows figures on indicators using dynamic indication technic
ISR(TIMER2_COMPA_vect) {
  byte dimmingThreshold = getIndicatorDimmingThreshold(currentIndicator);
  if (++indicatorBrightnessCounters[currentIndicator] >= dimmingThreshold) {
    digitalWrite(INDICATOR_PINS[currentIndicator], LOW);
  }

  if (indicatorBrightnessCounters[currentIndicator] >= INDICATOR_SWITCH_THRESHOLD) {
    indicatorBrightnessCounters[currentIndicator] = 0;
    if (++currentIndicator > 3) {
      currentIndicator = 0;
    }

    if (getIndicatorDimmingThreshold(currentIndicator) > 0) {
      byte mask = FIGURE_MASKS[figures[currentIndicator]];
      digitalWrite(DECODER3_PIN, bitRead(mask, 0));
      digitalWrite(DECODER1_PIN, bitRead(mask, 1));
      digitalWrite(DECODER0_PIN, bitRead(mask, 2));
      digitalWrite(DECODER2_PIN, bitRead(mask, 3));

      digitalWrite(INDICATOR_PINS[currentIndicator], indicatorStates[currentIndicator]);
    }
  }
}
