#include <Arduino.h>
#include <Wire.h>

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
constexpr unsigned long OLED_INTERVAL_MS = 500;
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
const char *lastKeyEvent = "NONE";
unsigned long lastKeyAt = 0;
unsigned long lastKeyEventAt = 0;
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

const char *focusShortName(ThresholdFocus focus) {
  switch (focus) {
    case ThresholdFocus::LIGHT:
      return "LIGHT";
    case ThresholdFocus::TEMPERATURE:
      return "TEMP";
    case ThresholdFocus::SOUND:
      return "SOUND";
    case ThresholdFocus::AWAY_DELAY:
      return "AWAY";
  }
  return "LIGHT";
}

String focusedThresholdValue() {
  switch (thresholdFocus) {
    case ThresholdFocus::LIGHT:
      return String(thresholds.light);
    case ThresholdFocus::TEMPERATURE:
      return String(static_cast<int>(thresholds.temperature + 0.5f));
    case ThresholdFocus::SOUND:
      return String(thresholds.sound);
    case ThresholdFocus::AWAY_DELAY:
      return String(thresholds.awayDelaySeconds);
  }
  return String(thresholds.light);
}

String displayLine(uint8_t index) {
  switch (index) {
    case 0:
      return "N16R8 PRIMARY";
    case 1:
      return String("MODE:") + modeDisplayName(currentMode);
    case 2:
      return String("FOCUS:") + focusShortName(thresholdFocus) + " " + focusedThresholdValue();
    case 3:
      return String("L:") + sensors.light + " T:" +
             (sensors.dhtValid ? String(static_cast<int>(sensors.temperature + 0.5f)) : "--") + " S:" +
             sensors.sound;
    case 4:
      return String("KEY:") + lastKeyEvent + " RAW:" + sensors.keypadRaw;
    case 5:
      return String("PIR:") + (sensors.pir ? "ON" : "OFF") + " FAN:" + actuators.fan;
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
  lastKeyEvent = key;
  lastKeyEventAt = now;

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

bool applyThresholdFocusCommand(const String &line) {
  if (line.indexOf("\"thresholdFocus\":\"lightThreshold\"") >= 0) {
    thresholdFocus = ThresholdFocus::LIGHT;
    return true;
  }
  if (line.indexOf("\"thresholdFocus\":\"temperatureThreshold\"") >= 0) {
    thresholdFocus = ThresholdFocus::TEMPERATURE;
    return true;
  }
  if (line.indexOf("\"thresholdFocus\":\"soundThreshold\"") >= 0) {
    thresholdFocus = ThresholdFocus::SOUND;
    return true;
  }
  if (line.indexOf("\"thresholdFocus\":\"awayDelaySeconds\"") >= 0) {
    thresholdFocus = ThresholdFocus::AWAY_DELAY;
    return true;
  }
  return false;
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
  if (applyThresholdFocusCommand(line)) {
    handled = true;
    message = "set=thresholdFocus";
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

void emitDisplayJson() {
  Serial.print("\"display\":{\"lines\":[");
  for (uint8_t index = 0; index < Oled::TEXT_LINES; index++) {
    if (index > 0) Serial.print(",");
    Serial.print("\"");
    Serial.print(displayLine(index));
    Serial.print("\"");
  }
  Serial.print("],\"lastKey\":\"");
  Serial.print(lastKeyEvent);
  Serial.print("\",\"focus\":\"");
  Serial.print(focusName(thresholdFocus));
  Serial.print("\"}");
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
  Serial.print(",\"keypadKey\":\"");
  Serial.print(lastKeyEvent);
  Serial.print("\"");
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
  Serial.print("\"},");
  emitDisplayJson();
  Serial.print(",\"health\":{\"profileId\":\"");
  Serial.print(PROFILE_ID);
  Serial.print("\",\"mqtt\":\"bridge\",\"voice\":\"ready\",\"thresholdFocus\":\"");
  Serial.print(focusName(thresholdFocus));
  Serial.print("\",\"keypadLastKey\":\"");
  Serial.print(lastKeyEvent);
  Serial.print("\",\"keypadLastMs\":");
  Serial.print(lastKeyEventAt);
  Serial.print(",\"oled\":\"");
  Serial.print(oledReady ? "ready" : "missing");
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
  if (now - lastSensorAt >= SENSOR_INTERVAL_MS) {
    readSensors();
    processKeypad();
    applyAutomation();
    lastSensorAt = now;
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
