#include <IBusBM.h>

// #define TRACING

#define MOTOR_A 0
#define MOTOR_B 1

#define MOTOR_STATE_BRAKE    0
#define MOTOR_STATE_FORWARD  1
#define MOTOR_STATE_REVERSE -1

#define CHANNEL_MIN_VALUE 1000
#define CHANNEL_MAX_VALUE 2000
#define CHANNEL_CORRECTION 40

#define STICK_DEAD_ZONE_MIN CHANNEL_MIN_VALUE + (CHANNEL_MAX_VALUE - CHANNEL_MIN_VALUE) / 2 - 15
#define STICK_DEAD_ZONE_MAX CHANNEL_MIN_VALUE + (CHANNEL_MAX_VALUE - CHANNEL_MIN_VALUE) / 2 + 15

#define BATTERY_STATUS_OK         1
#define BATTERY_STATUS_LOW        2
#define BATTERY_STATUS_DISCHARGED 3

#define ENA_PIN  3
#define ENB_PIN  5
#define MC1A_PIN 7
#define MC2A_PIN 6
#define MC1B_PIN 2
#define MC2B_PIN 4

#define CH1_PIN 10 
#define CH2_PIN 11
#define CH5_PIN 12

#define LIGHTS_PIN 13

#define BATTERY_SENSOR_PIN A0

#define BATTERY_INDICATOR_R_PIN 8
#define BATTERY_INDICATOR_G_PIN 9

IBusBM IBus;

int motorStates[2] = {MOTOR_STATE_BRAKE, MOTOR_STATE_BRAKE};
unsigned long batteryTimer;
bool lightsOn;
int batteryStatus;

void setup() {
  Serial.begin(115200);

  IBus.begin(Serial);
  IBus.addSensor(IBUSS_EXTV);

  pinMode(ENA_PIN, OUTPUT);
  pinMode(ENB_PIN, OUTPUT);
  pinMode(MC1A_PIN, OUTPUT);
  pinMode(MC2A_PIN, OUTPUT);
  pinMode(MC1B_PIN, OUTPUT);
  pinMode(MC2B_PIN, OUTPUT);
  pinMode(LIGHTS_PIN, OUTPUT);
  pinMode(BATTERY_INDICATOR_R_PIN, OUTPUT);
  pinMode(BATTERY_INDICATOR_G_PIN, OUTPUT);

  driveOrBrakeMotor(MOTOR_A, 0);
  driveOrBrakeMotor(MOTOR_B, 0);

  updateBattery();
  if (batteryStatus == BATTERY_STATUS_DISCHARGED) {
    while (true) {}
  }
}

void loop() {
  IBus.loop();

  updateMotors();
  updateLights();
  updateBattery();

  #ifdef TRACING
  delay(500);
  #endif
}

void updateMotors() {
  int x = readStick(CH1_PIN, -255, 255);
  int y = readStick(CH2_PIN, -255, 255);

  int rightDuty = constrain(y + x, -255, 255);
  int leftDuty = constrain(y - x, -255, 255);

  #ifdef TRACING
  Serial.print("Right duty: ");
  Serial.println(rightDuty);
  Serial.print("Left duty: ");
  Serial.println(leftDuty);
  #endif

  driveOrBrakeMotor(MOTOR_A, leftDuty);
  driveOrBrakeMotor(MOTOR_B, rightDuty);
}

void updateLights() {
  if (lightsOn != readSwitch(CH5_PIN)) {
    lightsOn = !lightsOn;
    if (lightsOn) {
      digitalWrite(LIGHTS_PIN, HIGH);
    } else {
      digitalWrite(LIGHTS_PIN, LOW);
    }
  }
}

void updateBattery() {
  unsigned long now = millis();
  if (now - batteryTimer >= 100) {
    batteryTimer = now;

    int status;
    float voltage = readBatteryVoltage();
    if (voltage > 6.5) {
      status = BATTERY_STATUS_OK;
    } else if (voltage > 6.0) {
      status = BATTERY_STATUS_LOW;
    } else {
      status = BATTERY_STATUS_DISCHARGED;
    }

    setBatteryStatus(status);
    IBus.setSensorMeasurement(1, voltage * 100);
  }
}

float readBatteryVoltage() {
  int readNumber = 32;
  float sum = 0;
  float f = 5.0 / 1024.0 * 3.0;
  for (int i = 0; i < readNumber; i++) {
    sum += (float) analogRead(BATTERY_SENSOR_PIN) * f;
  }
  return round(sum / (float) readNumber * 10.0) / 10.0;
}

void setBatteryStatus(int status) {
  if (batteryStatus != status) {
    batteryStatus = status;
    if (status == BATTERY_STATUS_OK) {
      digitalWrite(BATTERY_INDICATOR_R_PIN, LOW);
      digitalWrite(BATTERY_INDICATOR_G_PIN, HIGH);
    } else if (status == BATTERY_STATUS_LOW) {
      digitalWrite(BATTERY_INDICATOR_R_PIN, HIGH);
      analogWrite(BATTERY_INDICATOR_G_PIN, 128);
    } else {
      digitalWrite(BATTERY_INDICATOR_R_PIN, HIGH);
      digitalWrite(BATTERY_INDICATOR_G_PIN, LOW);
    }
  }
}

void driveOrBrakeMotor(int motor, int duty) {
  int enPin, mc1Pin, mc2Pin;
  if (motor == MOTOR_A) {
    enPin = ENA_PIN;
    mc1Pin = MC1A_PIN;
    mc2Pin = MC2A_PIN;
  } else {
    enPin = ENB_PIN;
    mc1Pin = MC1B_PIN;
    mc2Pin = MC2B_PIN;
  }

  int newState = duty == 0 ? MOTOR_STATE_BRAKE : (duty > 0 ? MOTOR_STATE_FORWARD : MOTOR_STATE_REVERSE);
  if (newState != motorStates[motor]) {
    if (newState == MOTOR_STATE_BRAKE) {
      digitalWrite(enPin, LOW);
      digitalWrite(mc1Pin, LOW);
      digitalWrite(mc2Pin, LOW);
    } else if (newState == MOTOR_STATE_FORWARD) {
      digitalWrite(enPin, LOW);
      digitalWrite(mc1Pin, HIGH);
      digitalWrite(mc2Pin, LOW);
    } else {
      digitalWrite(enPin, LOW);
      digitalWrite(mc1Pin, LOW);
      digitalWrite(mc2Pin, HIGH);
    }

    motorStates[motor] = newState;
  }

  if (motor == MOTOR_B) {
    duty = compensateGearTrain(duty);
  }
  analogWrite(enPin, abs(duty));
}

int compensateGearTrain(int duty) {
  if (duty == 0) {
    return 0;
  }

  float f = 40.0 - (float) abs(duty) * 0.16;
  if (duty > 0) {
    return min(duty + f, 255);
  }
  return max(duty - f, -255);
}

int readChannel(int channel) {
  int val = pulseIn(channel, HIGH, 30000);
  
  #ifdef TRACING
  Serial.print("Channel ");
  if (channel == CH1_PIN) {
    Serial.print("1");
  } else if (channel == CH2_PIN) {
    Serial.print("2");
  } else {
    Serial.print("3");
  }
  Serial.print(": ");
  Serial.println(val);
  #endif

  if (val == 0) {
    return 0;
  }
  return constrain(val + CHANNEL_CORRECTION, CHANNEL_MIN_VALUE, CHANNEL_MAX_VALUE);
}

int readStick(int channel, int minVal, int maxVal) {
  int val = readChannel(channel);
  if (val == 0 || (val >= STICK_DEAD_ZONE_MIN && val <= STICK_DEAD_ZONE_MAX)) {
    return 0;
  }
  return map(val, CHANNEL_MIN_VALUE, CHANNEL_MAX_VALUE, minVal, maxVal);
}

bool readSwitch(int channel) {
  return map(readChannel(channel), CHANNEL_MIN_VALUE, CHANNEL_MAX_VALUE, 0, 100) > 50;
}
