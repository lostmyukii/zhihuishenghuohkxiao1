#include <Arduino.h>

#if __has_include(<esp_arduino_version.h>)
#include <esp_arduino_version.h>
#endif

#if __has_include(<esp32-hal-rgb-led.h>)
#include <esp32-hal-rgb-led.h>
#define HAS_NEOPIXEL_WRITE 1
#else
#define HAS_NEOPIXEL_WRITE 0
#endif

#ifndef ESP_ARDUINO_VERSION_MAJOR
#define ESP_ARDUINO_VERSION_MAJOR 2
#endif

namespace Pins {
constexpr uint8_t LIGHT = 1;
constexpr uint8_t MQ2 = 2;
constexpr uint8_t KEYPAD = 3;
constexpr uint8_t SOUND = 4;
constexpr uint8_t PIR = 5;
constexpr uint8_t FLAME = 6;
constexpr uint8_t WATER = 8;
constexpr uint8_t SERVO = 9;
constexpr uint8_t FAN_PWM = 11;
constexpr uint8_t FAN_DIR = 12;
constexpr uint8_t BUZZER = 13;
constexpr uint8_t DHT = 14;
constexpr uint8_t OLED_SDA = 41;
constexpr uint8_t OLED_SCL = 42;
constexpr uint8_t RGB = 47;
constexpr uint8_t LAMP = 48;
}  // namespace Pins

namespace Pwm {
constexpr uint8_t FAN_CHANNEL = 0;
constexpr uint8_t BUZZER_CHANNEL = 1;
constexpr uint8_t SERVO_CHANNEL = 2;
constexpr uint8_t FAN_RESOLUTION = 8;
constexpr uint8_t BUZZER_RESOLUTION = 8;
constexpr uint8_t SERVO_RESOLUTION = 16;
constexpr uint32_t FAN_FREQ = 5000;
constexpr uint32_t BUZZER_FREQ = 2200;
constexpr uint32_t SERVO_FREQ = 50;
}  // namespace Pwm

enum class Mode {
  STUDY,
  REST,
  AWAY,
  ENERGY,
};

enum class ThresholdFocus {
  LIGHT,
  TEMPERATURE,
  SOUND,
  AWAY_DELAY,
};

struct Sensors {
  int lightRaw = 0;
  int light = 0;
  int soundRaw = 0;
  int sound = 0;
  int keypadRaw = 4095;
  bool pir = false;
  bool water = false;
  bool flame = false;
  bool mq2Alert = false;
  bool dhtValid = false;
  float temperature = NAN;
  float humidity = NAN;
};

struct Actuators {
  bool lamp = false;
  int fan = 0;
  int curtain = 45;
  const char *rgb = "safe";
  bool buzzer = false;
};

struct Thresholds {
  int light = 35;
  float temperature = 29.0f;
  int sound = 65;
  int awayDelaySeconds = 10;
};

constexpr const char *PROJECT = "smartlife-primary";
constexpr const char *PROFILE_ID = "smartlife-primary-study-home-v1";
constexpr const char *FIRMWARE_VERSION = "0.1.0";
constexpr unsigned long TELEMETRY_INTERVAL_MS = 1000;
constexpr unsigned long SENSOR_INTERVAL_MS = 200;
constexpr unsigned long MANUAL_OVERRIDE_MS = 10000;

Sensors sensors;
Actuators actuators;
Thresholds thresholds;
Mode currentMode = Mode::STUDY;
ThresholdFocus thresholdFocus = ThresholdFocus::LIGHT;
String serialBuffer;
unsigned long lastTelemetryAt = 0;
unsigned long lastSensorAt = 0;
unsigned long manualOverrideUntil = 0;
const char *lastKey = "NONE";
unsigned long lastKeyAt = 0;

int clampInt(int value, int minValue, int maxValue) {
  if (value < minValue) return minValue;
  if (value > maxValue) return maxValue;
  return value;
}

const char *modeName(Mode mode) {
  switch (mode) {
    case Mode::STUDY:
      return "study";
    case Mode::REST:
      return "rest";
    case Mode::AWAY:
      return "away";
    case Mode::ENERGY:
      return "energy";
  }
  return "study";
}

const char *focusName(ThresholdFocus focus) {
  switch (focus) {
    case ThresholdFocus::LIGHT:
      return "lightThreshold";
    case ThresholdFocus::TEMPERATURE:
      return "temperatureThreshold";
    case ThresholdFocus::SOUND:
      return "soundThreshold";
    case ThresholdFocus::AWAY_DELAY:
      return "awayDelaySeconds";
  }
  return "lightThreshold";
}

void attachPwm(uint8_t pin, uint8_t channel, uint32_t freq, uint8_t resolution) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcAttach(pin, freq, resolution);
#else
  ledcSetup(channel, freq, resolution);
  ledcAttachPin(pin, channel);
#endif
}

void writePwm(uint8_t pin, uint8_t channel, uint32_t duty) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWrite(pin, duty);
#else
  ledcWrite(channel, duty);
#endif
}

void setFan(int percent) {
  actuators.fan = clampInt(percent, 0, 100);
  digitalWrite(Pins::FAN_DIR, actuators.fan > 0 ? HIGH : LOW);
  writePwm(Pins::FAN_PWM, Pwm::FAN_CHANNEL, map(actuators.fan, 0, 100, 0, 255));
}

void setBuzzer(bool on) {
  actuators.buzzer = on;
  writePwm(Pins::BUZZER, Pwm::BUZZER_CHANNEL, on ? 128 : 0);
}

void setLamp(bool on) {
  actuators.lamp = on;
  digitalWrite(Pins::LAMP, on ? HIGH : LOW);
}

void setCurtain(int angle) {
  actuators.curtain = clampInt(angle, 0, 180);
  const uint32_t micros = map(actuators.curtain, 0, 180, 500, 2400);
  const uint32_t duty = (micros * 65535UL) / 20000UL;
  writePwm(Pins::SERVO, Pwm::SERVO_CHANNEL, duty);
}

void setRgb(const char *color) {
  actuators.rgb = color;
  uint8_t red = 2;
  uint8_t green = 2;
  uint8_t blue = 2;
  if (strcmp(color, "off") == 0) {
    red = 0;
    green = 0;
    blue = 0;
  } else if (strcmp(color, "blue") == 0) {
    red = 0;
    green = 0;
    blue = 16;
  } else if (strcmp(color, "amber") == 0) {
    red = 16;
    green = 8;
    blue = 0;
  } else if (strcmp(color, "red") == 0) {
    red = 18;
    green = 0;
    blue = 0;
  } else if (strcmp(color, "green") == 0) {
    red = 0;
    green = 16;
    blue = 0;
  }
#if HAS_NEOPIXEL_WRITE
  neopixelWrite(Pins::RGB, red, green, blue);
#else
  digitalWrite(Pins::RGB, (red || green || blue) ? HIGH : LOW);
#endif
}

void setBootSafeOutputs() {
  setLamp(false);
  setFan(0);
  setCurtain(45);
  setRgb("safe");
  setBuzzer(false);
}

bool readDht11(float &humidity, float &temperature) {
  uint8_t data[5] = {0, 0, 0, 0, 0};

  pinMode(Pins::DHT, OUTPUT_OPEN_DRAIN);
  digitalWrite(Pins::DHT, LOW);
  delay(20);
  digitalWrite(Pins::DHT, HIGH);
  delayMicroseconds(40);
  pinMode(Pins::DHT, INPUT_PULLUP);

  if (pulseIn(Pins::DHT, LOW, 1000) == 0) return false;
  if (pulseIn(Pins::DHT, HIGH, 1000) == 0) return false;

  for (uint8_t bit = 0; bit < 40; bit++) {
    const unsigned long lowTime = pulseIn(Pins::DHT, LOW, 1000);
    const unsigned long highTime = pulseIn(Pins::DHT, HIGH, 1000);
    if (lowTime == 0 || highTime == 0) return false;
    data[bit / 8] <<= 1;
    if (highTime > lowTime) {
      data[bit / 8] |= 1;
    }
  }

  const uint8_t checksum = data[0] + data[1] + data[2] + data[3];
  if (checksum != data[4]) return false;

  humidity = data[0] + data[1] * 0.1f;
  temperature = data[2] + data[3] * 0.1f;
  return true;
}

int percentFromAnalog(int raw) {
  return clampInt(map(raw, 0, 4095, 0, 100), 0, 100);
}

void readSensors() {
  sensors.lightRaw = analogRead(Pins::LIGHT);
  sensors.soundRaw = analogRead(Pins::SOUND);
  sensors.keypadRaw = analogRead(Pins::KEYPAD);
  sensors.light = percentFromAnalog(sensors.lightRaw);
  sensors.sound = percentFromAnalog(sensors.soundRaw);
  sensors.pir = digitalRead(Pins::PIR) == HIGH;
  sensors.water = digitalRead(Pins::WATER) == HIGH;
  sensors.flame = digitalRead(Pins::FLAME) == LOW;
  sensors.mq2Alert = analogRead(Pins::MQ2) > 3000;

  float humidity = NAN;
  float temperature = NAN;
  sensors.dhtValid = readDht11(humidity, temperature);
  if (sensors.dhtValid) {
    sensors.humidity = humidity;
    sensors.temperature = temperature;
  }
}

const char *decodeKeypad(int raw) {
  if (raw > 3900) return "NONE";
  if (raw < 300) return "A";
  if (raw < 800) return "B";
  if (raw < 1300) return "C";
  if (raw < 1800) return "D";
  if (raw < 2300) return "LEFT";
  if (raw < 2800) return "RIGHT";
  if (raw < 3300) return "UP";
  if (raw < 3800) return "DOWN";
  return "NONE";
}

void adjustFocusedThreshold(int direction) {
  switch (thresholdFocus) {
    case ThresholdFocus::LIGHT:
      thresholds.light = clampInt(thresholds.light + direction * 5, 0, 100);
      break;
    case ThresholdFocus::TEMPERATURE:
      thresholds.temperature = constrain(thresholds.temperature + direction * 0.5f, 10.0f, 40.0f);
      break;
    case ThresholdFocus::SOUND:
      thresholds.sound = clampInt(thresholds.sound + direction * 5, 0, 100);
      break;
    case ThresholdFocus::AWAY_DELAY:
      thresholds.awayDelaySeconds = clampInt(thresholds.awayDelaySeconds + direction * 5, 0, 120);
      break;
  }
}

void cycleThresholdFocus(int direction) {
  int index = static_cast<int>(thresholdFocus) + direction;
  if (index < 0) index = 3;
  if (index > 3) index = 0;
  thresholdFocus = static_cast<ThresholdFocus>(index);
}

void setMode(Mode mode) {
  currentMode = mode;
  manualOverrideUntil = 0;
}

void processKeypad() {
  const char *key = decodeKeypad(sensors.keypadRaw);
  const unsigned long now = millis();
  if (strcmp(key, "NONE") == 0) {
    lastKey = "NONE";
    return;
  }
  if (strcmp(key, lastKey) == 0 && now - lastKeyAt < 400) {
    return;
  }
  lastKey = key;
  lastKeyAt = now;

  if (strcmp(key, "A") == 0) setMode(Mode::STUDY);
  if (strcmp(key, "B") == 0) setMode(Mode::REST);
  if (strcmp(key, "C") == 0) setMode(Mode::AWAY);
  if (strcmp(key, "D") == 0) setMode(Mode::ENERGY);
  if (strcmp(key, "LEFT") == 0) cycleThresholdFocus(-1);
  if (strcmp(key, "RIGHT") == 0) cycleThresholdFocus(1);
  if (strcmp(key, "UP") == 0) adjustFocusedThreshold(1);
  if (strcmp(key, "DOWN") == 0) adjustFocusedThreshold(-1);
}

void applyAutomation() {
  if (manualOverrideUntil > millis()) {
    return;
  }

  const bool tempHigh = sensors.dhtValid && sensors.temperature >= thresholds.temperature;
  const bool soundHigh = sensors.sound >= thresholds.sound;
  const bool lightLow = sensors.light <= thresholds.light;

  switch (currentMode) {
    case Mode::STUDY:
      setLamp(lightLow);
      setFan(tempHigh ? 60 : 0);
      setCurtain(45);
      setRgb("blue");
      setBuzzer(soundHigh);
      break;
    case Mode::REST:
      setLamp(false);
      setFan(0);
      setCurtain(110);
      setRgb("amber");
      setBuzzer(false);
      break;
    case Mode::AWAY:
      setLamp(false);
      setFan(0);
      setCurtain(0);
      setRgb(sensors.pir ? "red" : "green");
      setBuzzer(sensors.pir);
      break;
    case Mode::ENERGY:
      setLamp(sensors.pir && lightLow);
      setFan(sensors.pir && tempHigh ? 40 : 0);
      setCurtain(lightLow ? 45 : 110);
      setRgb("green");
      setBuzzer(false);
      break;
  }
}

int parseIntAfter(const String &line, const char *key, int fallback) {
  const int start = line.indexOf(key);
  if (start < 0) return fallback;
  const int valueStart = start + strlen(key);
  return line.substring(valueStart).toInt();
}

bool parseBoolField(const String &line, const char *key, bool fallback) {
  const int start = line.indexOf(key);
  if (start < 0) return fallback;
  const int valueStart = start + strlen(key);
  const String tail = line.substring(valueStart);
  if (tail.startsWith("true")) return true;
  if (tail.startsWith("false")) return false;
  return fallback;
}

void emitAck(bool ok, const String &message) {
  Serial.print("{\"type\":\"ack\",\"ok\":");
  Serial.print(ok ? "true" : "false");
  Serial.print(",\"message\":\"");
  Serial.print(message);
  Serial.println("\"}");
}

void processCommand(String line) {
  line.trim();
  if (line.length() == 0) return;
  if (!line.startsWith("{")) return;

  bool handled = false;
  String message = "ok";

  if (line.indexOf("\"type\":\"ping\"") >= 0 || line.indexOf("\"ping\"") >= 0) {
    handled = true;
    message = "pong";
  }

  if (line.indexOf("\"mode\":\"study\"") >= 0 || line.indexOf("startStudy") >= 0) {
    setMode(Mode::STUDY);
    handled = true;
    message = "mode=study";
  } else if (line.indexOf("\"mode\":\"rest\"") >= 0 || line.indexOf("startRest") >= 0) {
    setMode(Mode::REST);
    handled = true;
    message = "mode=rest";
  } else if (line.indexOf("\"mode\":\"away\"") >= 0 || line.indexOf("leaveHome") >= 0) {
    setMode(Mode::AWAY);
    handled = true;
    message = "mode=away";
  } else if (line.indexOf("\"mode\":\"energy\"") >= 0 || line.indexOf("energy") >= 0) {
    setMode(Mode::ENERGY);
    handled = true;
    message = "mode=energy";
  }

  if (line.indexOf("\"lightThreshold\":") >= 0) {
    thresholds.light = clampInt(parseIntAfter(line, "\"lightThreshold\":", thresholds.light), 0, 100);
    handled = true;
    message = "set=lightThreshold";
  }
  if (line.indexOf("\"temperatureThreshold\":") >= 0) {
    thresholds.temperature = constrain(
        parseIntAfter(line, "\"temperatureThreshold\":", static_cast<int>(thresholds.temperature)), 10, 40);
    handled = true;
    message = "set=temperatureThreshold";
  }
  if (line.indexOf("\"soundThreshold\":") >= 0) {
    thresholds.sound = clampInt(parseIntAfter(line, "\"soundThreshold\":", thresholds.sound), 0, 100);
    handled = true;
    message = "set=soundThreshold";
  }

  if (line.indexOf("\"actuator\"") >= 0) {
    manualOverrideUntil = millis() + MANUAL_OVERRIDE_MS;
    if (line.indexOf("\"lamp\":") >= 0) {
      setLamp(parseBoolField(line, "\"lamp\":", actuators.lamp));
      handled = true;
      message = "actuator=lamp";
    }
    if (line.indexOf("\"buzzer\":") >= 0) {
      setBuzzer(parseBoolField(line, "\"buzzer\":", actuators.buzzer));
      handled = true;
      message = "actuator=buzzer";
    }
    if (line.indexOf("\"fan\":") >= 0) {
      setFan(clampInt(parseIntAfter(line, "\"fan\":", actuators.fan), 0, 100));
      handled = true;
      message = "actuator=fan";
    }
    if (line.indexOf("\"curtain\":") >= 0) {
      setCurtain(clampInt(parseIntAfter(line, "\"curtain\":", actuators.curtain), 0, 180));
      handled = true;
      message = "actuator=curtain";
    }
  }

  if (line.indexOf("queryLight") >= 0) {
    handled = true;
    message = "query=light";
  }

  emitAck(handled, handled ? message : "unknown-command");
}

void emitFloatOrNull(float value, uint8_t decimals) {
  if (isnan(value)) {
    Serial.print("null");
  } else {
    Serial.print(value, decimals);
  }
}

void emitHello() {
  Serial.print("{\"type\":\"hello\",\"project\":\"");
  Serial.print(PROJECT);
  Serial.print("\",\"board\":\"n16r8_esp32s3\",\"profileId\":\"");
  Serial.print(PROFILE_ID);
  Serial.print("\",\"firmware\":\"");
  Serial.print(FIRMWARE_VERSION);
  Serial.print("\",\"deviceName\":\"N16R8 SmartLife Primary Study Home\",\"baud\":115200");
  Serial.print(",\"capabilities\":[\"webSerial\",\"mqttBridge\",\"dashboard\",\"voiceIntent\",\"energyScore\"]");
  Serial.print(",\"pins\":{\"light\":1,\"sound\":4,\"dht\":14,\"pir\":5,\"keypad\":3,\"lamp\":48");
  Serial.print(",\"fanPwm\":11,\"fanDir\":12,\"servo\":9,\"rgb\":47,\"buzzer\":13,\"oledSda\":41,\"oledScl\":42}");
  Serial.println("}");
}

void emitAlerts() {
  bool wrote = false;
  Serial.print("[");
  if (currentMode == Mode::AWAY && sensors.pir) {
    Serial.print("\"intrusion\"");
    wrote = true;
  }
  if (currentMode == Mode::STUDY && sensors.sound >= thresholds.sound) {
    if (wrote) Serial.print(",");
    Serial.print("\"noise\"");
    wrote = true;
  }
  if (sensors.mq2Alert) {
    if (wrote) Serial.print(",");
    Serial.print("\"mq2\"");
    wrote = true;
  }
  if (sensors.water) {
    if (wrote) Serial.print(",");
    Serial.print("\"water\"");
    wrote = true;
  }
  if (sensors.flame) {
    if (wrote) Serial.print(",");
    Serial.print("\"flame\"");
  }
  Serial.print("]");
}

int energyScore() {
  int score = 80;
  if (actuators.lamp && sensors.light > thresholds.light) score -= 20;
  if (actuators.fan > 0 && sensors.dhtValid && sensors.temperature < thresholds.temperature) score -= 15;
  if (!sensors.pir && (actuators.lamp || actuators.fan > 0)) score -= 25;
  if (currentMode == Mode::ENERGY) score += 10;
  return clampInt(score, 0, 100);
}

const char *energyReason() {
  if (!sensors.pir && !actuators.lamp && actuators.fan == 0) return "empty-room-outputs-off";
  if (currentMode == Mode::ENERGY) return "energy-mode-balances-light-and-comfort";
  if (currentMode == Mode::STUDY) return "study-mode-keeps-comfort-when-needed";
  return "scene-rules-active";
}

void emitTelemetry() {
  Serial.print("{\"type\":\"telemetry\",\"ts\":");
  Serial.print(millis());
  Serial.print(",\"mode\":\"");
  Serial.print(modeName(currentMode));
  Serial.print("\",\"sensors\":{\"light\":");
  Serial.print(sensors.light);
  Serial.print(",\"lightRaw\":");
  Serial.print(sensors.lightRaw);
  Serial.print(",\"sound\":");
  Serial.print(sensors.sound);
  Serial.print(",\"soundRaw\":");
  Serial.print(sensors.soundRaw);
  Serial.print(",\"temperature\":");
  emitFloatOrNull(sensors.temperature, 1);
  Serial.print(",\"humidity\":");
  emitFloatOrNull(sensors.humidity, 1);
  Serial.print(",\"pir\":");
  Serial.print(sensors.pir ? "true" : "false");
  Serial.print(",\"keypadRaw\":");
  Serial.print(sensors.keypadRaw);
  Serial.print("},\"actuators\":{\"lamp\":");
  Serial.print(actuators.lamp ? "true" : "false");
  Serial.print(",\"fan\":");
  Serial.print(actuators.fan);
  Serial.print(",\"curtain\":");
  Serial.print(actuators.curtain);
  Serial.print(",\"rgb\":\"");
  Serial.print(actuators.rgb);
  Serial.print("\",\"buzzer\":");
  Serial.print(actuators.buzzer ? "true" : "false");
  Serial.print("},\"alerts\":");
  emitAlerts();
  Serial.print(",\"energy\":{\"score\":");
  Serial.print(energyScore());
  Serial.print(",\"reason\":\"");
  Serial.print(energyReason());
  Serial.print("\"},\"health\":{\"profileId\":\"");
  Serial.print(PROFILE_ID);
  Serial.print("\",\"mqtt\":\"bridge\",\"voice\":\"ready\",\"thresholdFocus\":\"");
  Serial.print(focusName(thresholdFocus));
  Serial.print("\",\"dht\":\"");
  Serial.print(sensors.dhtValid ? "ok" : "missing");
  Serial.print("\"}}");
  Serial.println("}");
}

void processSerial() {
  while (Serial.available() > 0) {
    const char c = static_cast<char>(Serial.read());
    if (c == '\n') {
      processCommand(serialBuffer);
      serialBuffer = "";
    } else if (c != '\r') {
      serialBuffer += c;
      if (serialBuffer.length() > 512) {
        serialBuffer = "";
      }
    }
  }
}

void setupPins() {
  pinMode(Pins::PIR, INPUT);
  pinMode(Pins::WATER, INPUT);
  pinMode(Pins::FLAME, INPUT_PULLUP);
  pinMode(Pins::RGB, OUTPUT);
  pinMode(Pins::LAMP, OUTPUT);
  pinMode(Pins::FAN_DIR, OUTPUT);
  pinMode(Pins::MQ2, INPUT);
  pinMode(Pins::LIGHT, INPUT);
  pinMode(Pins::SOUND, INPUT);
  pinMode(Pins::KEYPAD, INPUT);

  analogReadResolution(12);
  attachPwm(Pins::FAN_PWM, Pwm::FAN_CHANNEL, Pwm::FAN_FREQ, Pwm::FAN_RESOLUTION);
  attachPwm(Pins::BUZZER, Pwm::BUZZER_CHANNEL, Pwm::BUZZER_FREQ, Pwm::BUZZER_RESOLUTION);
  attachPwm(Pins::SERVO, Pwm::SERVO_CHANNEL, Pwm::SERVO_FREQ, Pwm::SERVO_RESOLUTION);
  setBootSafeOutputs();
}

void setup() {
  setupPins();
  Serial.begin(115200);
  delay(300);
  emitHello();
}

void loop() {
  processSerial();

  const unsigned long now = millis();
  if (now - lastSensorAt >= SENSOR_INTERVAL_MS) {
    readSensors();
    processKeypad();
    applyAutomation();
    lastSensorAt = now;
  }

  if (now - lastTelemetryAt >= TELEMETRY_INTERVAL_MS) {
    emitTelemetry();
    lastTelemetryAt = now;
  }
}
