#include <Arduino.h>
#include <TinyGPSPlus.h>
#include <Wire.h>
#include "pins.h"

HardwareSerial lteSerial(1);
HardwareSerial gpsSerial(0);
TinyGPSPlus gpsParser;
bool lteSerialStarted = false;
bool gpsSerialStarted = false;
volatile uint32_t lteRxEdgeCount = 0;

void IRAM_ATTR countLteRxEdge() {
  lteRxEdgeCount++;
}

enum class SystemState {
  BOOT,
  NOT_WORN,
  NORMAL,
  SUSPECT,
  ALARM,
  EMERGENCY_READY,
  ERROR
};

enum class BuzzerMode {
  OFF,
  WORN_TONE,
  ALARM_PATTERN,
  ERROR_PATTERN
};

struct SensorSnapshot {
  float tempC = NAN;
  uint32_t red = 0;
  uint32_t ir = 0;
  bool tmpOk = false;
  bool maxOk = false;
  bool worn = false;
};

struct LteStatus {
  bool atOk = false;
  bool simReady = false;
  bool registered = false;
  bool attached = false;
  int csq = -1;
  String operatorName;
};

struct LocationFix {
  bool ok = false;
  float lat = NAN;
  float lon = NAN;
};

constexpr uint32_t SENSOR_SAMPLE_MS = 100;
constexpr uint32_t TEMP_SAMPLE_MS = 1000;
constexpr uint32_t STATUS_PRINT_MS = 1000;
constexpr uint32_t LTE_CHECK_MS = 60000;
constexpr uint32_t SUSPECT_CONFIRM_MS = 10000;
constexpr uint32_t ALARM_COUNTDOWN_MS = 30000;

constexpr uint32_t WEAR_ON_IR_THRESHOLD = 80000;
constexpr uint32_t WEAR_OFF_IR_THRESHOLD = 60000;
constexpr uint8_t WEAR_CONFIRM_COUNT = 3;

constexpr float TEMP_SUSPECT_C = 38.0f;

SystemState state = SystemState::BOOT;
BuzzerMode buzzerMode = BuzzerMode::OFF;
SensorSnapshot sensors;
LteStatus lteStatus;

bool max30102Ready = false;
uint8_t wornHighCount = 0;
uint8_t wornLowCount = 0;
uint8_t tmpFailCount = 0;
uint8_t maxFailCount = 0;

uint32_t lastSensorMs = 0;
uint32_t lastTempMs = 0;
uint32_t lastStatusMs = 0;
uint32_t lastLteCheckMs = 0;
uint32_t suspectStartMs = 0;
uint32_t alarmStartMs = 0;
uint32_t buzzerTickMs = 0;
bool buzzerOutput = false;

String serialLine;
bool standaloneTestRunning = false;
bool triggerWasLow = false;
uint32_t triggerLowSince = 0;
bool batteryTestRunning = false;
bool batteryTriggerWasLow = false;
uint32_t batteryTriggerLowSince = 0;

void pollSerialCommands();
void updateSensors();
void updateStateMachine();
void updateBuzzer();
void handleStandaloneTrigger();
void handleBatteryTestTrigger();
void beep(uint16_t durationMs);
void progressBeep(uint16_t durationMs = 45);
void pulseModemPowerKeyHigh(uint16_t highMs);
bool probeLteBaud(uint32_t baud);
void updateLteStatus(bool verbose);

const char *stateName(SystemState value) {
  switch (value) {
    case SystemState::BOOT: return "BOOT";
    case SystemState::NOT_WORN: return "NOT_WORN";
    case SystemState::NORMAL: return "NORMAL";
    case SystemState::SUSPECT: return "SUSPECT";
    case SystemState::ALARM: return "ALARM";
    case SystemState::EMERGENCY_READY: return "EMERGENCY_READY";
    case SystemState::ERROR: return "ERROR";
  }
  return "UNKNOWN";
}

void setState(SystemState next) {
  if (state == next) return;
  Serial.printf("[state] %s -> %s\n", stateName(state), stateName(next));
  state = next;

  if (next == SystemState::SUSPECT) {
    suspectStartMs = millis();
  } else if (next == SystemState::ALARM) {
    alarmStartMs = millis();
  }
}

void setBuzzerMode(BuzzerMode next) {
  if (buzzerMode == next) return;
  buzzerMode = next;
  buzzerTickMs = 0;
}

bool i2cDevicePresent(uint8_t address) {
  Wire.beginTransmission(address);
  return Wire.endTransmission() == 0;
}

void scanI2cBus() {
  Serial.println("[i2c] scanning...");
  uint8_t found = 0;
  for (uint8_t addr = 1; addr < 127; addr++) {
    if (i2cDevicePresent(addr)) {
      Serial.printf("[i2c] found 0x%02X\n", addr);
      found++;
    }
  }
  if (found == 0) {
    Serial.println("[i2c] no devices found");
  }
}

bool readRegister16(uint8_t address, uint8_t reg, uint16_t &value) {
  Wire.beginTransmission(address);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom(static_cast<int>(address), 2) != 2) return false;
  value = (static_cast<uint16_t>(Wire.read()) << 8) | Wire.read();
  return true;
}

bool writeRegister16(uint8_t address, uint8_t reg, uint16_t value) {
  Wire.beginTransmission(address);
  Wire.write(reg);
  Wire.write(static_cast<uint8_t>(value >> 8));
  Wire.write(static_cast<uint8_t>(value & 0xFF));
  return Wire.endTransmission() == 0;
}

void monitorIna219Power(uint16_t durationSeconds) {
  constexpr uint8_t INA219_ADDR = 0x40;
  constexpr float SHUNT_OHMS = 0.1f;

  if (!i2cDevicePresent(INA219_ADDR)) {
    Serial.println("[ina219] not found at 0x40");
    return;
  }

  // 32 V range, 320 mV shunt range, 12-bit continuous shunt and bus readings.
  if (!writeRegister16(INA219_ADDR, 0x00, 0x399F)) {
    Serial.println("[ina219] configuration failed");
    return;
  }

  delay(5);
  Serial.printf("[ina219] monitoring %u s; assuming R100 (0.1 ohm) shunt\n",
                durationSeconds);
  Serial.println("[ina219] ms,load_V,source_V,current_mA,shunt_mV,overflow");

  uint32_t startMs = millis();
  uint32_t samples = 0;
  float minLoadV = 1000.0f;
  float maxLoadV = -1000.0f;
  float minCurrentMa = 1000000.0f;
  float maxCurrentMa = -1000000.0f;
  double sumLoadV = 0.0;
  double sumCurrentMa = 0.0;

  while (millis() - startMs < static_cast<uint32_t>(durationSeconds) * 1000UL) {
    uint16_t shuntRawUnsigned = 0;
    uint16_t busRaw = 0;
    if (!readRegister16(INA219_ADDR, 0x01, shuntRawUnsigned) ||
        !readRegister16(INA219_ADDR, 0x02, busRaw)) {
      Serial.println("[ina219] read failed");
      delay(100);
      continue;
    }

    int16_t shuntRaw = static_cast<int16_t>(shuntRawUnsigned);
    float shuntMv = shuntRaw * 0.01f;
    float loadV = (busRaw >> 3) * 0.004f;
    float currentMa = (shuntMv / 1000.0f) / SHUNT_OHMS * 1000.0f;
    float sourceV = loadV + shuntMv / 1000.0f;
    bool overflow = (busRaw & 0x01) != 0;

    minLoadV = min(minLoadV, loadV);
    maxLoadV = max(maxLoadV, loadV);
    minCurrentMa = min(minCurrentMa, currentMa);
    maxCurrentMa = max(maxCurrentMa, currentMa);
    sumLoadV += loadV;
    sumCurrentMa += currentMa;
    samples++;

    Serial.printf("[ina219] %lu,%.3f,%.3f,%.1f,%.3f,%u\n",
                  millis() - startMs, loadV, sourceV, currentMa, shuntMv,
                  overflow);
    delay(100);
  }

  if (samples == 0) {
    Serial.println("[ina219] no valid samples");
    return;
  }

  Serial.printf(
      "[ina219-summary] samples=%lu load_V[min=%.3f avg=%.3f max=%.3f] "
      "current_mA[min=%.1f avg=%.1f max=%.1f]\n",
      samples, minLoadV, static_cast<float>(sumLoadV / samples), maxLoadV,
      minCurrentMa, static_cast<float>(sumCurrentMa / samples), maxCurrentMa);
}

bool writeRegister8(uint8_t address, uint8_t reg, uint8_t value) {
  Wire.beginTransmission(address);
  Wire.write(reg);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

bool readRegister8(uint8_t address, uint8_t reg, uint8_t &value) {
  Wire.beginTransmission(address);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom(static_cast<int>(address), 1) != 1) return false;
  value = Wire.read();
  return true;
}

bool readTmp117Celsius(float &celsius) {
  uint16_t raw = 0;
  if (!readRegister16(TMP117_ADDR, 0x00, raw)) return false;
  float value = static_cast<int16_t>(raw) * 0.0078125f;
  if (value < -40.0f || value > 125.0f) return false;
  celsius = value;
  return true;
}

bool readTmp117CelsiusReliable(float &celsius, uint8_t attempts = 6) {
  for (uint8_t i = 0; i < attempts; i++) {
    if (readTmp117Celsius(celsius)) return true;
    delay(60);
  }
  if (!isnan(sensors.tempC)) {
    celsius = sensors.tempC;
    return true;
  }
  return false;
}

bool readMax30102PartId(uint8_t &partId) {
  Wire.beginTransmission(MAX30102_ADDR);
  Wire.write(0xFF);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom(static_cast<int>(MAX30102_ADDR), 1) != 1) return false;
  partId = Wire.read();
  return true;
}

bool initMax30102() {
  if (!writeRegister8(MAX30102_ADDR, 0x09, 0x40)) return false;
  delay(100);

  uint8_t mode = 0xFF;
  for (uint8_t i = 0; i < 10; i++) {
    if (readRegister8(MAX30102_ADDR, 0x09, mode) && (mode & 0x40) == 0) break;
    delay(50);
  }

  if (!writeRegister8(MAX30102_ADDR, 0x04, 0x00)) return false;
  if (!writeRegister8(MAX30102_ADDR, 0x05, 0x00)) return false;
  if (!writeRegister8(MAX30102_ADDR, 0x06, 0x00)) return false;
  if (!writeRegister8(MAX30102_ADDR, 0x08, 0x4F)) return false;
  if (!writeRegister8(MAX30102_ADDR, 0x09, 0x03)) return false;
  if (!writeRegister8(MAX30102_ADDR, 0x0A, 0x27)) return false;
  if (!writeRegister8(MAX30102_ADDR, 0x0C, 0x24)) return false;
  if (!writeRegister8(MAX30102_ADDR, 0x0D, 0x24)) return false;
  return true;
}

bool readMax30102Sample(uint32_t &red, uint32_t &ir) {
  Wire.beginTransmission(MAX30102_ADDR);
  Wire.write(0x07);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom(static_cast<int>(MAX30102_ADDR), 6) != 6) return false;

  red = (static_cast<uint32_t>(Wire.read()) << 16) |
        (static_cast<uint32_t>(Wire.read()) << 8) |
        Wire.read();
  ir = (static_cast<uint32_t>(Wire.read()) << 16) |
       (static_cast<uint32_t>(Wire.read()) << 8) |
       Wire.read();
  red &= 0x3FFFF;
  ir &= 0x3FFFF;
  return true;
}

void updateWearState(uint32_t ir) {
  bool nextWorn = sensors.worn;

  if (ir >= WEAR_ON_IR_THRESHOLD) {
    wornHighCount = min<uint8_t>(WEAR_CONFIRM_COUNT, wornHighCount + 1);
    wornLowCount = 0;
  } else if (ir <= WEAR_OFF_IR_THRESHOLD) {
    wornLowCount = min<uint8_t>(WEAR_CONFIRM_COUNT, wornLowCount + 1);
    wornHighCount = 0;
  }

  if (!sensors.worn && wornHighCount >= WEAR_CONFIRM_COUNT) {
    nextWorn = true;
  } else if (sensors.worn && wornLowCount >= WEAR_CONFIRM_COUNT) {
    nextWorn = false;
  }

  if (nextWorn != sensors.worn) {
    sensors.worn = nextWorn;
    Serial.printf("[wear] %s ir=%lu\n", sensors.worn ? "detected" : "removed", ir);
  }
}

String readLteResponse(uint32_t timeoutMs) {
  String response;
  const uint32_t start = millis();
  while (millis() - start < timeoutMs) {
    while (lteSerial.available()) {
      response += static_cast<char>(lteSerial.read());
    }
    delay(5);
  }
  response.trim();
  return response;
}

String sendLteCommand(const char *command, uint32_t timeoutMs = 1500) {
  if (!lteSerialStarted) {
    lteSerial.begin(115200, SERIAL_8N1, LTE_RX_PIN, LTE_TX_PIN);
    lteSerialStarted = true;
    delay(100);
  }
  Serial.printf("[lte] >> %s\n", command);
  lteSerial.print(command);
  lteSerial.print("\r\n");
  String response = readLteResponse(timeoutMs);
  Serial.printf("[lte] << %s\n", response.length() ? response.c_str() : "no response");
  return response;
}

bool waitForLtePrompt(uint32_t timeoutMs) {
  const uint32_t start = millis();
  String response;
  while (millis() - start < timeoutMs) {
    while (lteSerial.available()) {
      char c = static_cast<char>(lteSerial.read());
      response += c;
      if (c == '>') {
        Serial.printf("[lte] << %s\n", response.c_str());
        return true;
      }
    }
    delay(5);
  }
  response.trim();
  Serial.printf("[lte] << prompt timeout%s%s\n", response.length() ? ": " : "", response.c_str());
  return false;
}

bool sendSms(const char *phoneNumber, const char *message) {
  if (!lteStatus.registered) {
    Serial.println("[sms] blocked: LTE is not registered");
    return false;
  }

  sendLteCommand("AT+CMGF=1", 3000);
  sendLteCommand("AT+CSCS=\"GSM\"", 3000);

  Serial.printf("[sms] sending to %s\n", phoneNumber);
  lteSerial.print("AT+CMGS=\"");
  lteSerial.print(phoneNumber);
  lteSerial.print("\"\r\n");

  if (!waitForLtePrompt(10000)) {
    Serial.println("[sms] failed: no prompt");
    return false;
  }

  lteSerial.print(message);
  lteSerial.write(0x1A);
  String response = readLteResponse(60000);
  Serial.printf("[sms] response: %s\n", response.length() ? response.c_str() : "no response");
  return response.indexOf("OK") >= 0 || response.indexOf("+CMGS") >= 0;
}

bool modemAtOk() {
  String response = sendLteCommand("AT", 1200);
  return response.indexOf("OK") >= 0;
}

bool waitForModemAt(const char *label, uint8_t attempts, uint32_t intervalMs) {
  for (uint8_t i = 1; i <= attempts; i++) {
    Serial.printf("[standalone] %s AT check %u/%u\n", label, i, attempts);
    lteSerial.end();
    delay(80);
    lteSerial.begin(115200, SERIAL_8N1, LTE_RX_PIN, LTE_TX_PIN);
    lteSerialStarted = true;
    delay(120);
    if (modemAtOk()) {
      Serial.printf("[standalone] modem responds during %s wait\n", label);
      return true;
    }
    if (i % 5 == 0) progressBeep(25);
    delay(intervalMs);
  }
  return false;
}

bool ensureModemAwake() {
  Serial.println("[standalone] quick AT check before PWRK");
  if (waitForModemAt("pre-pwrk", 2, 500)) {
    return true;
  }

  Serial.println("[standalone] quick direct AT failed; probing UART before PWRK");
  if (probeLteBaud(115200)) {
    updateLteStatus(true);
    return lteStatus.atOk;
  }

  Serial.println("[standalone] no AT response; pulsing PWRK HIGH for 1000 ms");
  pulseModemPowerKeyHigh(1000);
  Serial.println("[standalone] waiting 7 seconds for modem boot");
  delay(7000);

  if (waitForModemAt("post-pwrk", 8, 1500)) {
    return true;
  }

  Serial.println("[standalone] post-PWRK direct AT failed; probing UART again");
  if (probeLteBaud(115200)) {
    updateLteStatus(true);
    return lteStatus.atOk;
  }

  Serial.println("[standalone] modem did not respond");
  return false;
}

int parseCsq(const String &response) {
  int marker = response.indexOf("+CSQ:");
  if (marker < 0) return -1;
  int comma = response.indexOf(',', marker);
  if (comma < 0) return -1;
  String value = response.substring(marker + 5, comma);
  value.trim();
  return value.toInt();
}

bool responseHasRegistration(const String &response) {
  return response.indexOf(",1") >= 0 || response.indexOf(",5") >= 0;
}

bool responseHasGnssCoordinate(const String &response) {
  int marker = response.indexOf("+CGNSINF:");
  if (marker < 0) return false;

  int lineEnd = response.indexOf('\n', marker);
  String line = lineEnd >= 0 ? response.substring(marker, lineEnd) : response.substring(marker);
  line.trim();

  int colon = line.indexOf(':');
  if (colon < 0) return false;

  String payload = line.substring(colon + 1);
  payload.trim();

  String fields[6];
  int fieldIndex = 0;
  int start = 0;
  while (fieldIndex < 6) {
    int comma = payload.indexOf(',', start);
    if (comma < 0) {
      fields[fieldIndex++] = payload.substring(start);
      break;
    }
    fields[fieldIndex++] = payload.substring(start, comma);
    start = comma + 1;
  }

  if (fieldIndex < 5) return false;

  fields[0].trim();
  fields[3].trim();
  fields[4].trim();
  if (fields[0] != "1" || fields[3].length() == 0 || fields[4].length() == 0) return false;

  float lat = fields[3].toFloat();
  float lon = fields[4].toFloat();
  return lat >= -90.0f && lat <= 90.0f && lon >= -180.0f && lon <= 180.0f;
}

LocationFix parseGnssCoordinate(const String &response) {
  LocationFix fix;
  int marker = response.indexOf("+CGNSINF:");
  if (marker < 0) return fix;

  int lineEnd = response.indexOf('\n', marker);
  String line = lineEnd >= 0 ? response.substring(marker, lineEnd) : response.substring(marker);
  line.trim();

  int colon = line.indexOf(':');
  if (colon < 0) return fix;

  String payload = line.substring(colon + 1);
  payload.trim();

  String fields[6];
  int fieldIndex = 0;
  int start = 0;
  while (fieldIndex < 6) {
    int comma = payload.indexOf(',', start);
    if (comma < 0) {
      fields[fieldIndex++] = payload.substring(start);
      break;
    }
    fields[fieldIndex++] = payload.substring(start, comma);
    start = comma + 1;
  }

  if (fieldIndex < 5) return fix;

  fields[0].trim();
  fields[3].trim();
  fields[4].trim();
  if (fields[0] != "1" || fields[3].length() == 0 || fields[4].length() == 0) return fix;

  float lat = fields[3].toFloat();
  float lon = fields[4].toFloat();
  if (lat < -90.0f || lat > 90.0f || lon < -180.0f || lon > 180.0f) return fix;

  fix.ok = true;
  fix.lat = lat;
  fix.lon = lon;
  return fix;
}

void updateLteStatus(bool verbose = false) {
  String at = sendLteCommand("AT", 1500);
  lteStatus.atOk = at.indexOf("OK") >= 0;

  if (!lteStatus.atOk) {
    lteStatus.simReady = false;
    lteStatus.registered = false;
    lteStatus.attached = false;
    return;
  }

  String cpin = sendLteCommand("AT+CPIN?", 1500);
  String csq = sendLteCommand("AT+CSQ", 1500);
  String cereg = sendLteCommand("AT+CEREG?", 1500);
  String cops = sendLteCommand("AT+COPS?", 3000);
  String cgatt = sendLteCommand("AT+CGATT?", 1500);

  lteStatus.simReady = cpin.indexOf("READY") >= 0;
  lteStatus.csq = parseCsq(csq);
  lteStatus.registered = responseHasRegistration(cereg);
  lteStatus.attached = cgatt.indexOf("+CGATT: 1") >= 0;
  lteStatus.operatorName = cops;

  if (verbose) {
    Serial.printf("[lte-status] at=%u sim=%u reg=%u attach=%u csq=%d\n",
                  lteStatus.atOk, lteStatus.simReady, lteStatus.registered,
                  lteStatus.attached, lteStatus.csq);
  }
}

void configureKoreanCatM() {
  sendLteCommand("AT+CNMP=38", 3000);
  sendLteCommand("AT+CMNB=1", 3000);
  sendLteCommand("AT+CBANDCFG=\"CAT-M\",1,3,5", 3000);
}

void initializeLte() {
  if (!lteSerialStarted) {
    lteSerial.begin(115200, SERIAL_8N1, LTE_RX_PIN, LTE_TX_PIN);
    lteSerialStarted = true;
  }
  delay(1000);
  sendLteCommand("ATE0", 1500);
  sendLteCommand("AT+CMEE=2", 1500);
  configureKoreanCatM();
  sendLteCommand("AT+CEREG=2", 1500);
  sendLteCommand("AT+CGREG=2", 1500);
  sendLteCommand("AT+COPS=0", 10000);
  updateLteStatus(true);
}

void releaseModemPowerKey() {
#if LTE_PWRK_PIN >= 0
  pinMode(LTE_PWRK_PIN, INPUT);
#endif
}

void pulseModemPowerKey(uint16_t lowMs = 1800) {
#if LTE_PWRK_PIN >= 0
  Serial.printf("[pwrk] pulling GPIO%d LOW for %u ms\n", LTE_PWRK_PIN, lowMs);
  pinMode(LTE_PWRK_PIN, OUTPUT);
  digitalWrite(LTE_PWRK_PIN, LOW);
  delay(lowMs);
  releaseModemPowerKey();
  Serial.println("[pwrk] released");
#else
  Serial.println("[pwrk] LTE_PWRK_PIN is disabled");
#endif
}

void pulseModemPowerKeyHigh(uint16_t highMs = 1800) {
#if LTE_PWRK_PIN >= 0
  Serial.printf("[pwrk] driving GPIO%d HIGH for %u ms\n", LTE_PWRK_PIN, highMs);
  pinMode(LTE_PWRK_PIN, OUTPUT);
  digitalWrite(LTE_PWRK_PIN, HIGH);
  delay(highMs);
  releaseModemPowerKey();
  Serial.println("[pwrk] released");
#else
  Serial.println("[pwrk] LTE_PWRK_PIN is disabled");
#endif
}

void powerDownModem() {
  Serial.println("[pwrk] requesting modem power down");
  sendLteCommand("AT+CPOWD=1", 10000);
  lteStatus.atOk = false;
  lteStatus.simReady = false;
  lteStatus.registered = false;
  lteStatus.attached = false;
}

bool probeLteBaud(uint32_t baud) {
  Serial.printf("[lte-probe] trying baud=%lu\n", baud);
  lteSerial.end();
  delay(100);
  lteSerial.begin(baud, SERIAL_8N1, LTE_RX_PIN, LTE_TX_PIN);
  delay(300);

  for (uint8_t i = 0; i < 5; i++) {
    lteSerial.print("AT\r\n");
    String response = readLteResponse(1000);
    Serial.printf("[lte-probe] baud=%lu attempt=%u response=%s\n",
                  baud, i + 1, response.length() ? response.c_str() : "no response");
    if (response.indexOf("OK") >= 0) {
      sendLteCommand("ATE0", 1500);
      return true;
    }
    delay(300);
  }
  return false;
}

void probeLteUart() {
  static const uint32_t baudRates[] = {115200, 9600, 19200, 38400, 57600, 74880, 230400};
  for (uint8_t i = 0; i < sizeof(baudRates) / sizeof(baudRates[0]); i++) {
    if (probeLteBaud(baudRates[i])) {
      Serial.printf("[lte-probe] success baud=%lu\n", baudRates[i]);
      updateLteStatus(true);
      return;
    }
  }

  Serial.println("[lte-probe] no UART response at tested baud rates; restoring 115200");
  lteSerial.end();
  delay(100);
  lteSerial.begin(115200, SERIAL_8N1, LTE_RX_PIN, LTE_TX_PIN);
}

void probeLteLineLevels() {
  lteSerial.end();
  delay(100);

  pinMode(LTE_RX_PIN, INPUT_PULLUP);
  pinMode(LTE_TX_PIN, INPUT_PULLUP);
  delay(20);
  const int rxWithPullup = digitalRead(LTE_RX_PIN);
  const int txWithPullup = digitalRead(LTE_TX_PIN);

  pinMode(LTE_RX_PIN, INPUT_PULLDOWN);
  pinMode(LTE_TX_PIN, INPUT_PULLDOWN);
  delay(20);
  const int rxWithPulldown = digitalRead(LTE_RX_PIN);
  const int txWithPulldown = digitalRead(LTE_TX_PIN);

  pinMode(LTE_RX_PIN, INPUT);
  pinMode(LTE_TX_PIN, INPUT);
  delay(20);

  uint32_t rxHigh = 0;
  uint32_t txHigh = 0;
  uint32_t rxTransitions = 0;
  uint32_t txTransitions = 0;
  int previousRx = digitalRead(LTE_RX_PIN);
  int previousTx = digitalRead(LTE_TX_PIN);

  for (uint32_t i = 0; i < 2000; i++) {
    const int rx = digitalRead(LTE_RX_PIN);
    const int tx = digitalRead(LTE_TX_PIN);
    rxHigh += rx == HIGH;
    txHigh += tx == HIGH;
    rxTransitions += rx != previousRx;
    txTransitions += tx != previousTx;
    previousRx = rx;
    previousTx = tx;
    delayMicroseconds(250);
  }

  Serial.printf(
      "[uart-levels] GPIO%d(ESP_RX/Stamp_TX) high=%lu/2000 transitions=%lu\n",
      LTE_RX_PIN, rxHigh, rxTransitions);
  Serial.printf(
      "[uart-levels] GPIO%d pullup=%d pulldown=%d\n",
      LTE_RX_PIN, rxWithPullup, rxWithPulldown);
  Serial.printf(
      "[uart-levels] GPIO%d(ESP_TX/Stamp_RX) high=%lu/2000 transitions=%lu\n",
      LTE_TX_PIN, txHigh, txTransitions);
  Serial.printf(
      "[uart-levels] GPIO%d pullup=%d pulldown=%d\n",
      LTE_TX_PIN, txWithPullup, txWithPulldown);

  lteSerial.begin(115200, SERIAL_8N1, LTE_RX_PIN, LTE_TX_PIN);
}

void testLteTxLine() {
  lteSerial.end();
  delay(100);

  pinMode(LTE_TX_PIN, OUTPUT);
  Serial.printf("[uart-tx-test] GPIO%d -> Stamp RX, five LOW/HIGH cycles\n", LTE_TX_PIN);
  for (uint8_t cycle = 1; cycle <= 5; cycle++) {
    digitalWrite(LTE_TX_PIN, LOW);
    Serial.printf("[uart-tx-test] cycle=%u LOW for 2 s\n", cycle);
    delay(2000);

    digitalWrite(LTE_TX_PIN, HIGH);
    Serial.printf("[uart-tx-test] cycle=%u HIGH for 2 s\n", cycle);
    delay(2000);
  }

  lteSerial.begin(115200, SERIAL_8N1, LTE_RX_PIN, LTE_TX_PIN);
  Serial.println("[uart-tx-test] complete; UART restored at 115200");
}

void testLteRxActivity() {
  static const uint32_t baudRates[] = {115200, 9600, 19200, 38400, 57600, 74880, 230400};

  Serial.println("[uart-rx-test] counting GPIO transitions while sending AT");
  for (uint8_t baudIndex = 0;
       baudIndex < sizeof(baudRates) / sizeof(baudRates[0]);
       baudIndex++) {
    const uint32_t baud = baudRates[baudIndex];
    lteSerial.end();
    delay(100);
    lteSerial.begin(baud, SERIAL_8N1, LTE_RX_PIN, LTE_TX_PIN);
    while (lteSerial.available()) lteSerial.read();

    lteRxEdgeCount = 0;
    attachInterrupt(digitalPinToInterrupt(LTE_RX_PIN), countLteRxEdge, CHANGE);
    for (uint8_t attempt = 0; attempt < 10; attempt++) {
      lteSerial.print("AT\r\n");
      lteSerial.flush();
      delay(100);
    }
    delay(500);
    detachInterrupt(digitalPinToInterrupt(LTE_RX_PIN));

    uint32_t byteCount = 0;
    while (lteSerial.available()) {
      lteSerial.read();
      byteCount++;
    }
    Serial.printf("[uart-rx-test] baud=%lu edges=%lu bytes=%lu\n",
                  baud, lteRxEdgeCount, byteCount);
  }

  lteSerial.end();
  delay(100);
  lteSerial.begin(115200, SERIAL_8N1, LTE_RX_PIN, LTE_TX_PIN);
  Serial.println("[uart-rx-test] complete; UART restored at 115200");
}

void startGpsSerial() {
#if GPS_RX_PIN >= 0
  if (!gpsSerialStarted) {
    gpsSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
    gpsSerialStarted = true;
    Serial.printf("[gps] UART started rx=%d tx=%d baud=%lu\n",
                  GPS_RX_PIN, GPS_TX_PIN, static_cast<unsigned long>(GPS_BAUD));
  }
#endif
}

LocationFix getGnssFix(uint8_t attempts = 12, uint32_t intervalMs = 3000) {
  LocationFix fix;
#if GPS_RX_PIN < 0
  Serial.println("[gps] external GPS UART is disabled for this build");
  return fix;
#else
  startGpsSerial();
  const uint32_t timeoutMs = static_cast<uint32_t>(attempts) * intervalMs;
  const uint32_t start = millis();
  uint32_t byteCount = 0;
  Serial.printf("[gps] waiting up to %lu ms for BE-220 fix\n",
                static_cast<unsigned long>(timeoutMs));

  while (millis() - start < timeoutMs) {
    while (gpsSerial.available()) {
      gpsParser.encode(static_cast<char>(gpsSerial.read()));
      byteCount++;
    }

    const bool locationReady =
        gpsParser.location.isValid() && gpsParser.location.age() < 5000;
    const bool qualityReady = gpsParser.satellites.isValid() &&
                              gpsParser.satellites.value() > 0 &&
                              gpsParser.hdop.isValid();
    if (locationReady && qualityReady) {
      fix.ok = true;
      fix.lat = static_cast<float>(gpsParser.location.lat());
      fix.lon = static_cast<float>(gpsParser.location.lng());
      Serial.printf("[gps] fix lat=%.6f lon=%.6f bytes=%lu satellites=%lu hdop=%.2f\n",
                    fix.lat, fix.lon, static_cast<unsigned long>(byteCount),
                    static_cast<unsigned long>(gpsParser.satellites.value()),
                    gpsParser.hdop.hdop());
      return fix;
    }

    updateSensors();
    updateStateMachine();
    updateBuzzer();
    delay(10);
  }

  Serial.printf("[gps] no fix bytes=%lu chars_processed=%lu failed_checksum=%lu satellites=%lu hdop=%.2f\n",
                static_cast<unsigned long>(byteCount),
                static_cast<unsigned long>(gpsParser.charsProcessed()),
                static_cast<unsigned long>(gpsParser.failedChecksum()),
                static_cast<unsigned long>(gpsParser.satellites.value()),
                gpsParser.hdop.hdop());
  return fix;
#endif
}

void readGnssFix(uint8_t attempts = 12, uint32_t intervalMs = 3000) {
  getGnssFix(attempts, intervalMs);
}

void printGpsRaw(uint16_t seconds = 5) {
#if GPS_RX_PIN < 0
  Serial.println("[gps-raw] external GPS UART is disabled for this build");
#else
  startGpsSerial();
  const uint32_t durationMs = static_cast<uint32_t>(seconds) * 1000UL;
  const uint32_t start = millis();
  uint32_t byteCount = 0;
  Serial.printf("[gps-raw] begin seconds=%u\n", seconds);

  while (millis() - start < durationMs) {
    while (gpsSerial.available()) {
      const int value = gpsSerial.read();
      if (value >= 0) {
        Serial.write(static_cast<uint8_t>(value));
        byteCount++;
      }
    }
    delay(1);
  }

  Serial.printf("\n[gps-raw] end bytes=%lu\n", static_cast<unsigned long>(byteCount));
#endif
}

void probeGpsBaud() {
#if GPS_RX_PIN < 0
  Serial.println("[gps-probe] external GPS UART is disabled for this build");
#else
  const uint32_t baudRates[] = {4800, 9600, 19200, 38400, 57600, 115200};
  Serial.println("[gps-probe] testing common baud rates");

  for (uint32_t baud : baudRates) {
    if (gpsSerialStarted) {
      gpsSerial.end();
      gpsSerialStarted = false;
      delay(100);
    }
    gpsSerial.begin(baud, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
    gpsSerialStarted = true;
    delay(300);
    while (gpsSerial.available()) gpsSerial.read();

    const uint32_t start = millis();
    uint32_t bytes = 0;
    uint32_t printable = 0;
    uint32_t dollarSigns = 0;
    uint32_t lineFeeds = 0;
    while (millis() - start < 1800) {
      while (gpsSerial.available()) {
        const int value = gpsSerial.read();
        if (value < 0) continue;
        bytes++;
        if ((value >= 32 && value <= 126) || value == '\r' || value == '\n') printable++;
        if (value == '$') dollarSigns++;
        if (value == '\n') lineFeeds++;
      }
      delay(1);
    }

    const uint32_t printablePct = bytes ? (printable * 100UL / bytes) : 0;
    Serial.printf("[gps-probe] baud=%lu bytes=%lu printable=%lu%% dollar=%lu lines=%lu\n",
                  static_cast<unsigned long>(baud),
                  static_cast<unsigned long>(bytes),
                  static_cast<unsigned long>(printablePct),
                  static_cast<unsigned long>(dollarSigns),
                  static_cast<unsigned long>(lineFeeds));
  }

  gpsSerial.end();
  delay(100);
  gpsSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
  gpsSerialStarted = true;
  Serial.printf("[gps-probe] restored configured baud=%lu\n",
                static_cast<unsigned long>(GPS_BAUD));
#endif
}

void disableGnss() {
#if GPS_RX_PIN >= 0
  if (gpsSerialStarted) {
    gpsSerial.end();
    gpsSerialStarted = false;
    Serial.println("[gps] UART stopped; BE-220 remains powered");
  }
#endif
}

bool estimateHeartRateBpm(float &bpm) {
  static uint32_t samples[400];
  const uint16_t sampleCount = 100;
  const uint16_t sampleMs = 50;

  Serial.println("[hr] estimating BPM for 5 seconds; keep finger on MAX30102");

  if (!max30102Ready) {
    uint8_t partId = 0;
    if (readMax30102PartId(partId)) {
      max30102Ready = initMax30102();
      Serial.printf("[max30102] init %s part_id=0x%02X\n", max30102Ready ? "ok" : "failed", partId);
    }
  }

  if (!max30102Ready) {
    for (uint8_t i = 0; i < 5; i++) {
      delay(900);
      beep(25);
      delay(75);
    }
    return false;
  }

  uint64_t sum = 0;
  uint32_t minValue = UINT32_MAX;
  uint32_t maxValue = 0;
  uint16_t validSamples = 0;

  for (uint16_t i = 0; i < sampleCount; i++) {
    uint32_t red = 0;
    uint32_t ir = 0;
    const uint32_t start = millis();
    if (readMax30102Sample(red, ir)) {
      samples[validSamples++] = ir;
      sum += ir;
      if (ir < minValue) minValue = ir;
      if (ir > maxValue) maxValue = ir;
    }

    if (i > 0 && i % 40 == 0) {
      beep(25);
    }

    updateBuzzer();
    while (millis() - start < sampleMs) {
      delay(2);
    }
  }

  if (validSamples < 20) {
    Serial.printf("[hr] insufficient samples=%u\n", validSamples);
    return false;
  }

  const float mean = static_cast<float>(sum) / validSamples;
  const float amplitude = static_cast<float>(maxValue - minValue);
  if (mean < WEAR_ON_IR_THRESHOLD || amplitude < 300.0f) {
    Serial.printf("[hr] invalid signal mean=%.1f amp=%.1f\n", mean, amplitude);
    return false;
  }

  const float threshold = mean + amplitude * 0.12f;
  uint16_t peaks = 0;
  int lastPeak = -1000;
  const int minPeakDistance = 6;

  for (uint16_t i = 1; i < validSamples - 1; i++) {
    bool isPeak = samples[i] > samples[i - 1] &&
                  samples[i] >= samples[i + 1] &&
                  samples[i] > threshold &&
                  static_cast<int>(i) - lastPeak >= minPeakDistance;
    if (isPeak) {
      peaks++;
      lastPeak = i;
    }
  }

  bpm = peaks * (60000.0f / (validSamples * sampleMs));
  Serial.printf("[hr] samples=%u peaks=%u bpm=%.1f mean=%.1f amp=%.1f\n",
                validSamples, peaks, bpm, mean, amplitude);
  return bpm >= 40.0f && bpm <= 220.0f;
}

void sendAlertSms(const char *phoneNumber) {
  float tempC = NAN;
  bool tempOk = readTmp117CelsiusReliable(tempC);
  if (tempOk) sensors.tempC = tempC;

  float bpm = NAN;
  bool bpmOk = estimateHeartRateBpm(bpm);
  LocationFix fix = getGnssFix();
  disableGnss();
  updateLteStatus(true);

  String message = "SOS HELP REQUEST\n";
  message += "HR: ";
  message += bpmOk ? String(bpm, 1) + " bpm" : "N/A";
  message += "\nTEMP: ";
  message += tempOk ? String(tempC, 2) + " C" : "N/A";
  message += "\nLOC: ";
  if (fix.ok) {
    message += String(fix.lat, 6);
    message += ",";
    message += String(fix.lon, 6);
  } else {
    message += "N/A";
  }

  Serial.println("[alert] message preview:");
  Serial.println(message);
  bool ok = sendSms(phoneNumber, message.c_str());
  Serial.printf("[alert] SMS result=%s\n", ok ? "success" : "failed");
}

bool waitForLteReady(uint8_t attempts = 4, uint32_t delayMs = 3000) {
  for (uint8_t i = 1; i <= attempts; i++) {
    Serial.printf("[lte] readiness check %u/%u\n", i, attempts);
    updateLteStatus(true);
    if (lteStatus.atOk && lteStatus.simReady && lteStatus.registered && lteStatus.attached) {
      return true;
    }
    if (i % 3 == 0) progressBeep(25);
    delay(delayMs);
  }
  return false;
}

void sendPowerTestSms(const char *phoneNumber) {
  float tempC = NAN;
  bool tempOk = readTmp117CelsiusReliable(tempC);
  if (tempOk) sensors.tempC = tempC;

  LocationFix fix = getGnssFix();
  disableGnss();
  delay(5000);
  bool lteReady = waitForLteReady();

  String message = "SOS HELP REQUEST | HR: N/A | TEMP: ";
  message += tempOk ? String(tempC, 2) + " C" : "N/A";
  message += " | LOC: ";
  if (fix.ok) {
    message += String(fix.lat, 6);
    message += ",";
    message += String(fix.lon, 6);
  } else {
    message += "N/A";
  }

  Serial.println("[powertest] message preview:");
  Serial.println(message);

  if (!lteReady) {
    Serial.println("[powertest] LTE not ready after GNSS; trying SMS anyway is blocked");
    Serial.println("[powertest] SMS result=failed");
    return;
  }

  bool ok = sendSms(phoneNumber, message.c_str());
  Serial.printf("[powertest] SMS result=%s\n", ok ? "success" : "failed");
}

void beep(uint16_t durationMs) {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(durationMs);
  digitalWrite(BUZZER_PIN, LOW);
}

void beepCount(uint8_t count, uint16_t durationMs = 120, uint16_t gapMs = 140) {
  for (uint8_t i = 0; i < count; i++) {
    beep(durationMs);
    if (i + 1 < count) delay(gapMs);
  }
}

void progressBeep(uint16_t durationMs) {
  beep(durationMs);
  delay(80);
}

bool runBatterySelfTest() {
  if (batteryTestRunning || standaloneTestRunning) return false;

  batteryTestRunning = true;
  setBuzzerMode(BuzzerMode::OFF);
  Serial.println("[battery-test] start; no GPS and no SMS");
  beep(300);
  delay(300);

  const bool tmpPresent = i2cDevicePresent(TMP117_ADDR);
  const bool maxPresent = i2cDevicePresent(MAX30102_ADDR);
  Serial.printf("[battery-test] TMP117=%u MAX30102=%u\n", tmpPresent, maxPresent);
  if (!tmpPresent || !maxPresent) {
    Serial.println("[battery-test] fail: sensor I2C");
    beepCount(3, 100, 100);
    batteryTestRunning = false;
    return false;
  }

  if (!ensureModemAwake()) {
    Serial.println("[battery-test] fail: Stamp AT");
    beepCount(4, 100, 100);
    batteryTestRunning = false;
    return false;
  }

  sendLteCommand("ATE0", 1500);
  sendLteCommand("AT+CMEE=2", 1500);
  updateLteStatus(true);

  // Do not force operator reselection when the modem is already registered.
  // A fresh COPS=0 can temporarily detach a healthy modem and made the
  // standalone test report a false failure before SKT registration recovered.
  bool lteReady = lteStatus.atOk && lteStatus.simReady &&
                  lteStatus.registered && lteStatus.attached;
  if (!lteReady) {
    configureKoreanCatM();
    sendLteCommand("AT+CEREG=2", 1500);
    lteReady = waitForLteReady(10, 2000);
  }

  // Use operator reselection only as a recovery step and allow registration
  // enough time to settle after it is requested.
  if (!lteReady) {
    Serial.println("[battery-test] registration recovery: COPS=0");
    sendLteCommand("AT+COPS=0", 10000);
    lteReady = waitForLteReady(8, 2500);
  }

  if (!lteReady) {
    Serial.println("[battery-test] fail: LTE registration");
    beepCount(5, 100, 100);
    batteryTestRunning = false;
    return false;
  }

  Serial.println("[battery-test] PASS: sensors, Stamp AT, SKT registration, packet attach");
  beepCount(2, 220, 180);
  batteryTestRunning = false;
  return true;
}

bool runStandaloneAlert(const char *phoneNumber) {
  if (standaloneTestRunning) {
    Serial.println("[standalone] already running");
    return false;
  }

  standaloneTestRunning = true;
  Serial.println("[standalone] start");
  setBuzzerMode(BuzzerMode::OFF);
  beepCount(1);

  Serial.println("[standalone] place finger / wear sensor within 5 seconds");
  delay(5000);
  beepCount(1, 70, 80);

  float tempC = NAN;
  bool tempOk = readTmp117CelsiusReliable(tempC);
  if (tempOk) sensors.tempC = tempC;

  float bpm = NAN;
  bool bpmOk = estimateHeartRateBpm(bpm);
  beepCount(2);

  Serial.println("[standalone] modem/LTE step");
  progressBeep();
  if (!ensureModemAwake()) {
    Serial.println("[standalone] fail: modem AT unavailable");
    beepCount(4, 80, 80);
    standaloneTestRunning = false;
    return false;
  }

  sendLteCommand("ATE0", 1500);
  sendLteCommand("AT+CMEE=2", 1500);
  configureKoreanCatM();
  sendLteCommand("AT+CEREG=2", 1500);
  sendLteCommand("AT+CGREG=2", 1500);
  sendLteCommand("AT+COPS=0", 10000);

  Serial.println("[standalone] LTE registration step");
  progressBeep();
  bool lteReady = waitForLteReady(20, 3000);
  if (!lteReady) {
    Serial.println("[standalone] fail: LTE not ready");
    beepCount(5, 80, 80);
    standaloneTestRunning = false;
    return false;
  }

  Serial.println("[standalone] GNSS step");
  progressBeep();
  LocationFix fix = getGnssFix();
  disableGnss();
  waitForLteReady(4, 2000);

  String message = "SOS HELP REQUEST | HR: ";
  message += bpmOk ? String(bpm, 1) + " bpm" : "N/A";
  message += " | TEMP: ";
  message += tempOk ? String(tempC, 2) + " C" : "N/A";
  message += " | LOC: ";
  if (fix.ok) {
    message += String(fix.lat, 6);
    message += ",";
    message += String(fix.lon, 6);
  } else {
    message += "N/A";
  }

  Serial.println("[standalone] message preview:");
  Serial.println(message);

  Serial.println("[standalone] SMS step");
  progressBeep();
  bool ok = sendSms(phoneNumber, message.c_str());

  Serial.printf("[standalone] result=%s\n", ok ? "success" : "failed");
  if (ok) {
    beepCount(3);
  } else {
    Serial.println("[standalone] fail: SMS send failed");
    beepCount(6, 80, 80);
  }

  standaloneTestRunning = false;
  return ok;
}

void runMeasureOnly() {
  Serial.println("[measure] start");
  float tempC = NAN;
  bool tempOk = readTmp117CelsiusReliable(tempC);
  if (tempOk) sensors.tempC = tempC;

  float bpm = NAN;
  bool bpmOk = estimateHeartRateBpm(bpm);

  Serial.printf("[measure] temp_ok=%u temp=%.2f bpm_ok=%u bpm=%.1f worn=%u red=%lu ir=%lu max_ok=%u\n",
                tempOk, tempC, bpmOk, bpm, sensors.worn, sensors.red, sensors.ir, sensors.maxOk);
}

void updateBuzzer() {
  uint32_t now = millis();
  bool nextOutput = false;

  switch (buzzerMode) {
    case BuzzerMode::OFF:
      nextOutput = false;
      break;
    case BuzzerMode::WORN_TONE:
      nextOutput = (now - buzzerTickMs) < 120;
      if (now - buzzerTickMs > 1000) buzzerTickMs = now;
      break;
    case BuzzerMode::ALARM_PATTERN:
      nextOutput = ((now / 250) % 2) == 0;
      break;
    case BuzzerMode::ERROR_PATTERN:
      nextOutput = ((now / 1000) % 2) == 0 && ((now / 150) % 2) == 0;
      break;
  }

  if (nextOutput != buzzerOutput) {
    buzzerOutput = nextOutput;
    digitalWrite(BUZZER_PIN, buzzerOutput ? HIGH : LOW);
  }
}

void updateSensors() {
  uint32_t now = millis();

  if (now - lastTempMs >= TEMP_SAMPLE_MS) {
    lastTempMs = now;
    float tempC = NAN;
    if (readTmp117Celsius(tempC)) {
      sensors.tempC = tempC;
      sensors.tmpOk = true;
      tmpFailCount = 0;
    } else {
      tmpFailCount = min<uint8_t>(20, tmpFailCount + 1);
      sensors.tmpOk = tmpFailCount < 5;
    }
  }

  if (!max30102Ready) {
    uint8_t partId = 0;
    if (readMax30102PartId(partId)) {
      max30102Ready = initMax30102();
      Serial.printf("[max30102] init %s part_id=0x%02X\n", max30102Ready ? "ok" : "failed", partId);
    }
  }

  if (now - lastSensorMs >= SENSOR_SAMPLE_MS) {
    lastSensorMs = now;
    uint32_t red = 0;
    uint32_t ir = 0;
    if (max30102Ready && readMax30102Sample(red, ir)) {
      sensors.red = red;
      sensors.ir = ir;
      sensors.maxOk = true;
      maxFailCount = 0;
      updateWearState(sensors.ir);
    } else {
      maxFailCount = min<uint8_t>(50, maxFailCount + 1);
      sensors.maxOk = maxFailCount < 20;
    }
  }
}

bool suspectCondition() {
  return sensors.worn && sensors.tmpOk && sensors.tempC >= TEMP_SUSPECT_C;
}

void updateStateMachine() {
#if !ENABLE_STATE_BUZZER
  if (!standaloneTestRunning) {
    setBuzzerMode(BuzzerMode::OFF);
  }
  if (tmpFailCount >= 5 || maxFailCount >= 20) {
    setState(SystemState::ERROR);
    return;
  }
#else
  if (tmpFailCount >= 5 || maxFailCount >= 20) {
    setState(SystemState::ERROR);
    setBuzzerMode(BuzzerMode::ERROR_PATTERN);
    return;
  }
#endif

  if (!sensors.worn) {
    setState(SystemState::NOT_WORN);
#if ENABLE_STATE_BUZZER
    setBuzzerMode(BuzzerMode::OFF);
#endif
    return;
  }

  switch (state) {
    case SystemState::BOOT:
    case SystemState::NOT_WORN:
    case SystemState::ERROR:
      setState(SystemState::NORMAL);
#if ENABLE_STATE_BUZZER
      setBuzzerMode(BuzzerMode::OFF);
#endif
      break;
    case SystemState::NORMAL:
#if ENABLE_STATE_BUZZER
      setBuzzerMode(BuzzerMode::OFF);
#endif
      if (suspectCondition()) setState(SystemState::SUSPECT);
      break;
    case SystemState::SUSPECT:
      if (!suspectCondition()) {
        setState(SystemState::NORMAL);
      } else if (millis() - suspectStartMs >= SUSPECT_CONFIRM_MS) {
        setState(SystemState::ALARM);
      }
      break;
    case SystemState::ALARM:
#if ENABLE_STATE_BUZZER
      setBuzzerMode(BuzzerMode::ALARM_PATTERN);
#endif
      if (millis() - alarmStartMs >= ALARM_COUNTDOWN_MS) {
        setState(SystemState::EMERGENCY_READY);
      }
      break;
    case SystemState::EMERGENCY_READY:
#if ENABLE_STATE_BUZZER
      setBuzzerMode(BuzzerMode::ALARM_PATTERN);
#endif
      break;
  }
}

void printStatus() {
  uint32_t now = millis();
  if (now - lastStatusMs < STATUS_PRINT_MS) return;
  lastStatusMs = now;

  Serial.printf("[status] state=%s worn=%u temp=%.2f tmp_ok=%u red=%lu ir=%lu max_ok=%u lte_reg=%u csq=%d\n",
                stateName(state), sensors.worn, sensors.tempC, sensors.tmpOk,
                sensors.red, sensors.ir, sensors.maxOk, lteStatus.registered,
                lteStatus.csq);
}

void handleSerialCommand(String line) {
  line.trim();
  if (line.length() == 0) return;

  if (line == "HELP") {
    Serial.println("[cmd] HELP | STATUS | I2CSCAN | POWERMON [seconds] | MEASURE | LTE | LTEPROBE | UARTLEVELS | UARTTXTEST | UARTRXTEST | ATCMD <command> | ATLONG <seconds> <command> | GPS | GPSRAW [seconds] | GPSPROBE | GPSOFF | BATTERYTEST | MODEMPWR [ms] | PWRKHIGH [ms] | MODEMOFF | ALARM | CANCEL | ALERTSMS <number> | POWERTESTSMS <number> | STANDALONE [number] | SMS <number> <message>");
  } else if (line == "STATUS") {
    lastStatusMs = 0;
    printStatus();
  } else if (line == "I2CSCAN") {
    scanI2cBus();
  } else if (line == "POWERMON" || line.startsWith("POWERMON ")) {
    uint16_t seconds = 10;
    if (line.startsWith("POWERMON ")) {
      uint32_t requestedSeconds = line.substring(9).toInt();
      if (requestedSeconds >= 1 && requestedSeconds <= 120) {
        seconds = requestedSeconds;
      }
    }
    monitorIna219Power(seconds);
  } else if (line == "MEASURE") {
    runMeasureOnly();
  } else if (line == "LTE") {
    updateLteStatus(true);
  } else if (line == "LTEPROBE") {
    probeLteUart();
  } else if (line == "UARTLEVELS") {
    probeLteLineLevels();
  } else if (line == "UARTTXTEST") {
    testLteTxLine();
  } else if (line == "UARTRXTEST") {
    testLteRxActivity();
  } else if (line.startsWith("ATCMD ")) {
    String atCommand = line.substring(6);
    sendLteCommand(atCommand.c_str(), 5000);
  } else if (line.startsWith("ATLONG ")) {
    const int secondsEnd = line.indexOf(' ', 7);
    if (secondsEnd < 0) {
      Serial.println("[cmd] usage: ATLONG <seconds> <command>");
      return;
    }

    const uint32_t seconds = line.substring(7, secondsEnd).toInt();
    String atCommand = line.substring(secondsEnd + 1);
    atCommand.trim();
    if (seconds < 1 || seconds > 300 || atCommand.length() == 0) {
      Serial.println("[cmd] ATLONG seconds must be 1..300");
      return;
    }

    sendLteCommand(atCommand.c_str(), seconds * 1000UL);
  } else if (line == "GPS" || line == "GNSS") {
    readGnssFix();
  } else if (line == "GPSRAW" || line.startsWith("GPSRAW ")) {
    uint16_t seconds = 5;
    if (line.startsWith("GPSRAW ")) {
      const uint32_t requestedSeconds = line.substring(7).toInt();
      if (requestedSeconds >= 1 && requestedSeconds <= 30) {
        seconds = requestedSeconds;
      }
    }
    printGpsRaw(seconds);
  } else if (line == "GPSPROBE") {
    probeGpsBaud();
  } else if (line == "GPSOFF" || line == "GNSSOFF") {
    disableGnss();
  } else if (line == "MODEMPWR" || line.startsWith("MODEMPWR ")) {
    uint16_t lowMs = 1800;
    if (line.startsWith("MODEMPWR ")) {
      uint32_t requestedMs = line.substring(9).toInt();
      if (requestedMs >= 500 && requestedMs <= 10000) lowMs = requestedMs;
    }
    pulseModemPowerKey(lowMs);
  } else if (line == "PWRKHIGH" || line.startsWith("PWRKHIGH ")) {
    uint16_t highMs = 1800;
    if (line.startsWith("PWRKHIGH ")) {
      uint32_t requestedMs = line.substring(9).toInt();
      if (requestedMs >= 500 && requestedMs <= 10000) highMs = requestedMs;
    }
    pulseModemPowerKeyHigh(highMs);
  } else if (line == "MODEMOFF") {
    powerDownModem();
  } else if (line == "ALARM") {
    setState(SystemState::ALARM);
  } else if (line == "CANCEL") {
    setState(sensors.worn ? SystemState::NORMAL : SystemState::NOT_WORN);
    setBuzzerMode(BuzzerMode::OFF);
  } else if (line.startsWith("ALERTSMS ")) {
    String number = line.substring(9);
    number.trim();
    if (number.length() == 0) {
      Serial.println("[cmd] ALERTSMS usage: ALERTSMS <number>");
      return;
    }
    sendAlertSms(number.c_str());
  } else if (line.startsWith("POWERTESTSMS ")) {
    String number = line.substring(13);
    number.trim();
    if (number.length() == 0) {
      Serial.println("[cmd] POWERTESTSMS usage: POWERTESTSMS <number>");
      return;
    }
    sendPowerTestSms(number.c_str());
  } else if (line == "BATTERYTEST") {
    runBatterySelfTest();
  } else if (line == "STANDALONE" || line.startsWith("STANDALONE ")) {
    String number = STANDALONE_ALERT_NUMBER;
    if (line.startsWith("STANDALONE ")) {
      number = line.substring(11);
      number.trim();
    }
    runStandaloneAlert(number.c_str());
  } else if (line.startsWith("SMS ")) {
    int space = line.indexOf(' ', 4);
    if (space < 0) {
      Serial.println("[cmd] SMS usage: SMS <number> <message>");
      return;
    }
    String number = line.substring(4, space);
    String message = line.substring(space + 1);
    bool ok = sendSms(number.c_str(), message.c_str());
    Serial.printf("[cmd] SMS result=%s\n", ok ? "success" : "failed");
  } else {
    Serial.printf("[cmd] unknown: %s\n", line.c_str());
  }
}

void handleStandaloneTrigger() {
#if TEST_TRIGGER_PIN >= 0
  const bool low = digitalRead(TEST_TRIGGER_PIN) == LOW;
  const uint32_t now = millis();
  if (low && !triggerWasLow) {
    triggerLowSince = now;
  }
  if (low && !standaloneTestRunning && triggerLowSince != 0 && now - triggerLowSince >= 1500) {
    if (digitalRead(TEST_TRIGGER_PIN) == LOW) {
      triggerLowSince = 0;
      runStandaloneAlert(STANDALONE_ALERT_NUMBER);
    }
  }
  if (!low) {
    triggerLowSince = 0;
  }
  triggerWasLow = low;
#endif
}

void handleBatteryTestTrigger() {
#if BATTERY_TEST_PIN >= 0
  const bool low = digitalRead(BATTERY_TEST_PIN) == LOW;
  const uint32_t now = millis();
  if (low && !batteryTriggerWasLow) batteryTriggerLowSince = now;
  if (low && !batteryTestRunning && !standaloneTestRunning &&
      batteryTriggerLowSince != 0 && now - batteryTriggerLowSince >= 1500) {
    if (digitalRead(BATTERY_TEST_PIN) == LOW) {
      batteryTriggerLowSince = 0;
      runBatterySelfTest();
    }
  }
  if (!low) batteryTriggerLowSince = 0;
  batteryTriggerWasLow = low;
#endif
}

void pollSerialCommands() {
  while (Serial.available()) {
    char c = static_cast<char>(Serial.read());
    if (c == '\n' || c == '\r') {
      handleSerialCommand(serialLine);
      serialLine = "";
    } else {
      serialLine += c;
    }
  }
}

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(500);

  Serial.println();
  Serial.println("Heat Risk Alert Wristband integrated firmware");
  Serial.println("[safety] SMS is manual only. Use: SMS <number> <message>");
  Serial.printf("[pins] i2c sda=%d scl=%d, lte rx=%d tx=%d, gps rx=%d tx=%d, buzzer=%d, pwrk=%d\n",
                I2C_SDA_PIN, I2C_SCL_PIN, LTE_RX_PIN, LTE_TX_PIN,
                GPS_RX_PIN, GPS_TX_PIN, BUZZER_PIN, LTE_PWRK_PIN);

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
#if TEST_TRIGGER_PIN >= 0
  pinMode(TEST_TRIGGER_PIN, INPUT_PULLUP);
#endif
#if BATTERY_TEST_PIN >= 0
  pinMode(BATTERY_TEST_PIN, INPUT_PULLUP);
#endif
  releaseModemPowerKey();

  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  Wire.setClock(100000);
  Wire.setTimeOut(50);
  scanI2cBus();

  beep(80);
#if AUTO_LTE_INIT
  initializeLte();
#else
  lteSerial.begin(115200, SERIAL_8N1, LTE_RX_PIN, LTE_TX_PIN);
  lteSerialStarted = true;
  Serial.println("[lte] auto init disabled; use LTE or LTEPROBE command");
#endif
  setState(SystemState::BOOT);
}

void loop() {
  pollSerialCommands();
  handleStandaloneTrigger();
  handleBatteryTestTrigger();
  updateSensors();
  updateStateMachine();
  updateBuzzer();
  printStatus();

  if (millis() - lastLteCheckMs >= LTE_CHECK_MS) {
    lastLteCheckMs = millis();
    updateLteStatus(true);
  }
}
