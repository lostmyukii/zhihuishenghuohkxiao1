#include <Arduino.h>
#include <DHT.h>
#include <Wire.h>

#if __has_include(<esp_arduino_version.h>)
#include <esp_arduino_version.h>
#endif

#ifndef ESP_ARDUINO_VERSION_MAJOR
#define ESP_ARDUINO_VERSION_MAJOR 2
#endif

namespace Pins {
constexpr uint8_t LIGHT = 1;
constexpr uint8_t MQ2 = 2;
constexpr uint8_t SOUND = 4;
constexpr uint8_t PIR = 5;
constexpr uint8_t FLAME = 6;
constexpr uint8_t WATER = 8;
constexpr uint8_t BUZZER = 13;
constexpr uint8_t DHT = 14;
constexpr uint8_t OLED_SDA = 41;
constexpr uint8_t OLED_SCL = 42;
constexpr uint8_t LAMP = 12;
}  // namespace Pins

namespace Pwm {
constexpr uint8_t BUZZER_CHANNEL = 1;
constexpr uint8_t BUZZER_RESOLUTION = 8;
constexpr uint32_t BUZZER_FREQ = 2200;
}  // namespace Pwm

DHT dht(Pins::DHT, DHT11);

enum class Mode {
  STUDY,
  REST,
  AWAY,
  ENERGY,
};

struct Sensors {
  int lightRaw = 0;
  int light = 0;
  int soundRaw = 0;
  int sound = 0;
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
constexpr const char *FIRMWARE_VERSION = "0.1.2";
constexpr unsigned long TELEMETRY_INTERVAL_MS = 1000;
constexpr unsigned long SENSOR_INTERVAL_MS = 200;
constexpr unsigned long DHT_INTERVAL_MS = 2000;
constexpr unsigned long DHT_STALE_MS = 6000;
constexpr unsigned long OLED_INTERVAL_MS = 500;
constexpr unsigned long MANUAL_OVERRIDE_MS = 10000;

Sensors sensors;
Actuators actuators;
Thresholds thresholds;
Mode currentMode = Mode::STUDY;
String serialBuffer;
unsigned long lastTelemetryAt = 0;
unsigned long lastSensorAt = 0;
unsigned long lastDhtReadAt = 0;
unsigned long lastDhtSuccessAt = 0;
unsigned long manualOverrideUntil = 0;
unsigned long lastOledAt = 0;
bool oledReady = false;

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

const char *modeDisplayName(Mode mode) {
  switch (mode) {
    case Mode::STUDY:
      return "STUDY";
    case Mode::REST:
      return "REST";
    case Mode::AWAY:
      return "AWAY";
    case Mode::ENERGY:
      return "ENERGY";
  }
  return "STUDY";
}

String displayLine(uint8_t index) {
  switch (index) {
    case 0:
      return "N16R8 PRIMARY";
    case 1:
      return String("MODE:") + modeDisplayName(currentMode);
    case 2:
      return String("TH:L") + thresholds.light + " T" + static_cast<int>(thresholds.temperature + 0.5f) +
             " S" + thresholds.sound;
    case 3:
      return String("L:") + sensors.light + " T:" +
             (sensors.dhtValid ? String(static_cast<int>(sensors.temperature + 0.5f)) : "--") + " S:" +
             sensors.sound;
    case 4:
      return String("PIR:") + (sensors.pir ? "ON" : "OFF") + " LAMP:" + (actuators.lamp ? "ON" : "OFF");
    case 5:
      return String("TEMP:") +
             (sensors.dhtValid && sensors.temperature >= thresholds.temperature ? "HI" : "OK") + " BZ:" +
             (actuators.buzzer ? "ON" : "OFF");
  }
  return "";
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

void setBuzzer(bool on) {
  actuators.buzzer = on;
  writePwm(Pins::BUZZER, Pwm::BUZZER_CHANNEL, on ? 128 : 0);
}

void setLamp(bool on) {
  actuators.lamp = on;
  digitalWrite(Pins::LAMP, on ? HIGH : LOW);
}

void setBootSafeOutputs() {
  setLamp(false);
  setBuzzer(false);
}

int percentFromAnalog(int raw) {
  return clampInt(map(raw, 0, 4095, 0, 100), 0, 100);
}

namespace Oled {
constexpr uint8_t ADDRESS = 0x3C;
constexpr uint8_t WIDTH = 128;
constexpr uint8_t PAGES = 8;
constexpr uint8_t TEXT_LINES = 6;
constexpr uint8_t CHARS_PER_LINE = 21;
constexpr uint8_t DATA_CHUNK = 16;
}  // namespace Oled

const uint8_t OLED_FONT_DIGITS[10][5] = {
    {0x3E, 0x51, 0x49, 0x45, 0x3E},
    {0x00, 0x42, 0x7F, 0x40, 0x00},
    {0x42, 0x61, 0x51, 0x49, 0x46},
    {0x21, 0x41, 0x45, 0x4B, 0x31},
    {0x18, 0x14, 0x12, 0x7F, 0x10},
    {0x27, 0x45, 0x45, 0x45, 0x39},
    {0x3C, 0x4A, 0x49, 0x49, 0x30},
    {0x01, 0x71, 0x09, 0x05, 0x03},
    {0x36, 0x49, 0x49, 0x49, 0x36},
    {0x06, 0x49, 0x49, 0x29, 0x1E},
};

const uint8_t OLED_FONT_UPPER[26][5] = {
    {0x7E, 0x11, 0x11, 0x11, 0x7E},
    {0x7F, 0x49, 0x49, 0x49, 0x36},
    {0x3E, 0x41, 0x41, 0x41, 0x22},
    {0x7F, 0x41, 0x41, 0x22, 0x1C},
    {0x7F, 0x49, 0x49, 0x49, 0x41},
    {0x7F, 0x09, 0x09, 0x09, 0x01},
    {0x3E, 0x41, 0x49, 0x49, 0x7A},
    {0x7F, 0x08, 0x08, 0x08, 0x7F},
    {0x00, 0x41, 0x7F, 0x41, 0x00},
    {0x20, 0x40, 0x41, 0x3F, 0x01},
    {0x7F, 0x08, 0x14, 0x22, 0x41},
    {0x7F, 0x40, 0x40, 0x40, 0x40},
    {0x7F, 0x02, 0x0C, 0x02, 0x7F},
    {0x7F, 0x04, 0x08, 0x10, 0x7F},
    {0x3E, 0x41, 0x41, 0x41, 0x3E},
    {0x7F, 0x09, 0x09, 0x09, 0x06},
    {0x3E, 0x41, 0x51, 0x21, 0x5E},
    {0x7F, 0x09, 0x19, 0x29, 0x46},
    {0x46, 0x49, 0x49, 0x49, 0x31},
    {0x01, 0x01, 0x7F, 0x01, 0x01},
    {0x3F, 0x40, 0x40, 0x40, 0x3F},
    {0x1F, 0x20, 0x40, 0x20, 0x1F},
    {0x7F, 0x20, 0x18, 0x20, 0x7F},
    {0x63, 0x14, 0x08, 0x14, 0x63},
    {0x03, 0x04, 0x78, 0x04, 0x03},
    {0x61, 0x51, 0x49, 0x45, 0x43},
};

const uint8_t OLED_GLYPH_SPACE[5] = {0x00, 0x00, 0x00, 0x00, 0x00};
const uint8_t OLED_GLYPH_COLON[5] = {0x00, 0x36, 0x36, 0x00, 0x00};
const uint8_t OLED_GLYPH_DASH[5] = {0x08, 0x08, 0x08, 0x08, 0x08};
const uint8_t OLED_GLYPH_SLASH[5] = {0x20, 0x10, 0x08, 0x04, 0x02};
const uint8_t OLED_GLYPH_DOT[5] = {0x00, 0x60, 0x60, 0x00, 0x00};
const uint8_t OLED_GLYPH_QUESTION[5] = {0x02, 0x01, 0x51, 0x09, 0x06};

const uint8_t *oledGlyphForChar(char value) {
  if (value >= 'a' && value <= 'z') value = static_cast<char>(value - 32);
  if (value >= '0' && value <= '9') return OLED_FONT_DIGITS[value - '0'];
  if (value >= 'A' && value <= 'Z') return OLED_FONT_UPPER[value - 'A'];
  if (value == ' ') return OLED_GLYPH_SPACE;
  if (value == ':') return OLED_GLYPH_COLON;
  if (value == '-') return OLED_GLYPH_DASH;
  if (value == '/') return OLED_GLYPH_SLASH;
  if (value == '.') return OLED_GLYPH_DOT;
  return OLED_GLYPH_QUESTION;
}

bool oledWriteTransmission(uint8_t control, const uint8_t *data, size_t length) {
  Wire.beginTransmission(Oled::ADDRESS);
  Wire.write(control);
  for (size_t index = 0; index < length; index++) {
    Wire.write(data[index]);
  }
  return Wire.endTransmission() == 0;
}

bool oledCommand(uint8_t command) {
  return oledWriteTransmission(0x00, &command, 1);
}

bool oledData(const uint8_t *data, size_t length) {
  size_t offset = 0;
  while (offset < length) {
    size_t chunk = length - offset;
    if (chunk > Oled::DATA_CHUNK) chunk = Oled::DATA_CHUNK;
    if (!oledWriteTransmission(0x40, data + offset, chunk)) return false;
    offset += chunk;
  }
  return true;
}

bool oledSetCursor(uint8_t page, uint8_t column) {
  if (!oledCommand(static_cast<uint8_t>(0xB0 | (page & 0x07)))) return false;
  if (!oledCommand(static_cast<uint8_t>(0x00 | (column & 0x0F)))) return false;
  return oledCommand(static_cast<uint8_t>(0x10 | ((column >> 4) & 0x0F)));
}

void oledClear() {
  if (!oledReady) return;
  const uint8_t empty[Oled::DATA_CHUNK] = {0};
  for (uint8_t page = 0; page < Oled::PAGES; page++) {
    if (!oledSetCursor(page, 0)) {
      oledReady = false;
      return;
    }
    for (uint8_t column = 0; column < Oled::WIDTH; column += Oled::DATA_CHUNK) {
      if (!oledData(empty, Oled::DATA_CHUNK)) {
        oledReady = false;
        return;
      }
    }
  }
}

void oledBegin() {
  Wire.begin(Pins::OLED_SDA, Pins::OLED_SCL);
  Wire.setClock(400000);
  delay(20);

  Wire.beginTransmission(Oled::ADDRESS);
  oledReady = Wire.endTransmission() == 0;
  if (!oledReady) return;

  const uint8_t initCommands[] = {
      0xAE, 0x20, 0x00, 0xB0, 0xC8, 0x00, 0x10, 0x40, 0x81, 0x7F,
      0xA1, 0xA6, 0xA8, 0x3F, 0xA4, 0xD3, 0x00, 0xD5, 0x80, 0xD9,
      0xF1, 0xDA, 0x12, 0xDB, 0x40, 0x8D, 0x14, 0xAF,
  };
  for (uint8_t index = 0; index < sizeof(initCommands); index++) {
    if (!oledCommand(initCommands[index])) {
      oledReady = false;
      return;
    }
  }
  oledClear();
}

void oledWriteTextLine(uint8_t row, const String &text) {
  if (!oledReady || row >= Oled::TEXT_LINES) return;
  if (!oledSetCursor(row, 0)) {
    oledReady = false;
    return;
  }

  for (uint8_t index = 0; index < Oled::CHARS_PER_LINE; index++) {
    char value = index < text.length() ? text.charAt(index) : ' ';
    const uint8_t *glyph = oledGlyphForChar(value);
    uint8_t columns[6] = {glyph[0], glyph[1], glyph[2], glyph[3], glyph[4], 0x00};
    if (!oledData(columns, sizeof(columns))) {
      oledReady = false;
      return;
    }
  }
}

void oledRenderStatus() {
  if (!oledReady) return;
  for (uint8_t row = 0; row < Oled::TEXT_LINES; row++) {
    oledWriteTextLine(row, displayLine(row));
    if (!oledReady) return;
  }
}

void readFastSensors() {
  sensors.lightRaw = analogRead(Pins::LIGHT);
  sensors.soundRaw = analogRead(Pins::SOUND);
  sensors.light = percentFromAnalog(sensors.lightRaw);
  sensors.sound = percentFromAnalog(sensors.soundRaw);
  sensors.pir = digitalRead(Pins::PIR) == HIGH;
  sensors.water = digitalRead(Pins::WATER) == HIGH;
  sensors.flame = digitalRead(Pins::FLAME) == LOW;
  sensors.mq2Alert = analogRead(Pins::MQ2) > 3000;
}

void readDhtSensor(unsigned long now) {
  const float humidity = dht.readHumidity();
  const float temperature = dht.readTemperature();
  if (!isnan(humidity) && !isnan(temperature)) {
    sensors.humidity = humidity;
    sensors.temperature = temperature;
    sensors.dhtValid = true;
    lastDhtSuccessAt = now;
    return;
  }

  if (lastDhtSuccessAt == 0 || now - lastDhtSuccessAt >= DHT_STALE_MS) {
    sensors.dhtValid = false;
  }
}

void setMode(Mode mode) {
  currentMode = mode;
  manualOverrideUntil = 0;
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
      setBuzzer(soundHigh || tempHigh);
      break;
    case Mode::REST:
      setLamp(false);
      setBuzzer(false);
      break;
    case Mode::AWAY:
      setLamp(false);
      setBuzzer(sensors.pir);
      break;
    case Mode::ENERGY:
      setLamp(sensors.pir && lightLow);
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
  if (line.indexOf("\"awayDelaySeconds\":") >= 0) {
    thresholds.awayDelaySeconds = clampInt(
        parseIntAfter(line, "\"awayDelaySeconds\":", thresholds.awayDelaySeconds), 0, 120);
    handled = true;
    message = "set=awayDelaySeconds";
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
  Serial.print(",\"pins\":{\"light\":1,\"sound\":4,\"dht\":14,\"pir\":5,\"lamp\":12");
  Serial.print(",\"buzzer\":13,\"oledSda\":41,\"oledScl\":42}");
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
  if (currentMode == Mode::STUDY && sensors.dhtValid && sensors.temperature >= thresholds.temperature) {
    if (wrote) Serial.print(",");
    Serial.print("\"temperature\"");
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
  if (!sensors.pir && actuators.lamp) score -= 25;
  if (currentMode == Mode::ENERGY) score += 10;
  return clampInt(score, 0, 100);
}

const char *energyReason() {
  if (!sensors.pir && !actuators.lamp) return "empty-room-light-off";
  if (currentMode == Mode::ENERGY) return "energy-mode-balances-light-and-comfort";
  if (currentMode == Mode::STUDY) return "study-mode-keeps-comfort-when-needed";
  return "scene-rules-active";
}

void emitDisplayJson() {
  Serial.print("\"display\":{\"lines\":[");
  for (uint8_t index = 0; index < Oled::TEXT_LINES; index++) {
    if (index > 0) Serial.print(",");
    Serial.print("\"");
    Serial.print(displayLine(index));
    Serial.print("\"");
  }
  Serial.print("]}");
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
  Serial.print("},\"actuators\":{\"lamp\":");
  Serial.print(actuators.lamp ? "true" : "false");
  Serial.print(",\"buzzer\":");
  Serial.print(actuators.buzzer ? "true" : "false");
  Serial.print("},\"alerts\":");
  emitAlerts();
  Serial.print(",\"energy\":{\"score\":");
  Serial.print(energyScore());
  Serial.print(",\"reason\":\"");
  Serial.print(energyReason());
  Serial.print("\"},");
  emitDisplayJson();
  Serial.print(",\"health\":{\"profileId\":\"");
  Serial.print(PROFILE_ID);
  Serial.print("\",\"mqtt\":\"bridge\",\"voice\":\"ready\",\"oled\":\"");
  Serial.print(oledReady ? "ready" : "missing");
  Serial.print("\",\"dht\":\"");
  Serial.print(sensors.dhtValid ? "ok" : "missing");
  Serial.print("\",\"buzzerEnabled\":true,\"relaySafety\":\"lowVoltageOnly\"}");
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
  pinMode(Pins::LAMP, OUTPUT);
  pinMode(Pins::MQ2, INPUT);
  pinMode(Pins::LIGHT, INPUT);
  pinMode(Pins::SOUND, INPUT);

  analogReadResolution(12);
  dht.begin();
  attachPwm(Pins::BUZZER, Pwm::BUZZER_CHANNEL, Pwm::BUZZER_FREQ, Pwm::BUZZER_RESOLUTION);
  setBootSafeOutputs();
  oledBegin();
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
  bool sensorUpdated = false;
  if (now - lastSensorAt >= SENSOR_INTERVAL_MS) {
    readFastSensors();
    lastSensorAt = now;
    sensorUpdated = true;
  }

  if (lastDhtReadAt == 0 || now - lastDhtReadAt >= DHT_INTERVAL_MS) {
    readDhtSensor(now);
    lastDhtReadAt = now;
    sensorUpdated = true;
  }

  if (sensorUpdated) {
    applyAutomation();
  }

  if (now - lastOledAt >= OLED_INTERVAL_MS) {
    oledRenderStatus();
    lastOledAt = now;
  }

  if (now - lastTelemetryAt >= TELEMETRY_INTERVAL_MS) {
    emitTelemetry();
    lastTelemetryAt = now;
  }
}
