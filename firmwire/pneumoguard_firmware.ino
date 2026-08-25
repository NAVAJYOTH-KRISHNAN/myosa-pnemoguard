/*
  =====================================================================
  PneumoGuard - AI-Assisted COPD Exacerbation Early Warning System
  IEEE MYOSA 6.0 - Team: Ribin K V, Navajyoth Krishnan D, Niveditha Ranjish
  Platform: MYOSA Mini (ESP32)
  =====================================================================

  Pipeline (matches the 4-stage design in the proposal):
    Stage 1 - Data Collection   : BMP180 (pressure/temp), MPU6050 (motion),
                                   APDS-9960 (ambient light)
    Stage 2 - Signal Processing : sliding window -> pressure-drop rate,
                                   breathing-effort index (accel variance)
    Stage 3 - Risk Scoring      : weighted fusion -> LOW / MEDIUM / HIGH
    Stage 4 - Alert & Delivery  : OLED + buzzer locally, Wi-Fi POST to
                                   cloud dashboard for caregiver review

  IMPORTANT - library changes from the original draft:
    On this hardware, Adafruit_MPU6050 and Adafruit_APDS9960 failed to
    detect their chips via begin(), even though an I2C scanner confirmed
    both chips are physically present and responding (MPU6050 at 0x69,
    APDS-9960 at 0x39 - both non-default addresses for their respective
    Adafruit libraries). This version talks to both sensors directly via
    raw I2C registers instead, which was verified working in isolation.
    BMP180 kept on Adafruit_BMP085, which worked fine as a library.

  Required libraries (Arduino Library Manager):
    - Adafruit BMP085 Library      (works for BMP180)
    - Adafruit SSD1306 + Adafruit GFX   (for OLED - not yet tested)
    - Adafruit Unified Sensor
    - ArduinoJson
    (Wire.h is built-in; MPU6050 and APDS-9960 need NO extra library)

  STATUS NOTES:
    - BMP180, MPU6050, APDS-9960: individually verified + verified
      running together on shared I2C bus.
    - OLED, buzzer pin, Wi-Fi/cloud logging: NOT yet tested on this
      hardware - wired up here per the proposal's design but treat these
      sections as unverified until you test them one at a time, the same
      way we did the sensors.
    - TESTING MODE is on below (short window) so you can validate logic
      quickly. Switch to deployment settings (commented) once confirmed.
  =====================================================================
*/

#include <Wire.h>
#include <Adafruit_BMP085.h>
#include <U8g2lib.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// ---------------------------------------------------------------------
// USER CONFIG
// ---------------------------------------------------------------------
const char* WIFI_SSID     = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
const char* DASHBOARD_URL = "https://your-dashboard.example.com/api/log";

const int BUZZER_PIN = 25;   // adjust to MYOSA Mini's actual buzzer pin - not yet confirmed

// ---- TESTING settings (fast iteration) ----
const unsigned long SAMPLE_INTERVAL_MS = 5UL * 1000UL;    // one reading per 5 sec
const int WINDOW_SIZE = 24;                                // 24 samples = 2-minute window
// ---- DEPLOYMENT settings (uncomment these two lines and comment the two above when ready) ----
// const unsigned long SAMPLE_INTERVAL_MS = 60UL * 1000UL;  // one reading per minute
// const int WINDOW_SIZE = 30;                               // 30 samples = 30-minute window

// Risk thresholds (tune with real/clinical data later)
const float PRESSURE_DROP_HPA_MEDIUM = 2.0;   // hPa drop over window -> medium concern
const float PRESSURE_DROP_HPA_HIGH   = 4.0;   // hPa drop over window -> high concern
const float ACCEL_VAR_MEDIUM         = 0.15;
const float ACCEL_VAR_HIGH           = 0.35;

// ---------------------------------------------------------------------
// SENSOR ADDRESSES (confirmed via I2C scanner on this hardware)
// ---------------------------------------------------------------------
const uint8_t MPU_ADDR  = 0x69;
const uint8_t APDS_ADDR = 0x39;

const uint8_t APDS_REG_ENABLE  = 0x80;
const uint8_t APDS_REG_ATIME   = 0x81;
const uint8_t APDS_REG_CONTROL = 0x8F;
const uint8_t APDS_REG_CDATAL  = 0x94;

// MPU6050 accel sensitivity at default power-on range (+/-2g): 16384 LSB/g
const float MPU_LSB_PER_G = 16384.0;
const float G_TO_MS2 = 9.80665;

// ---------------------------------------------------------------------
// OBJECTS
// ---------------------------------------------------------------------
Adafruit_BMP085 bmp;

// Confirmed via hardware testing: this panel is an SH1106, not the SSD1306
// the datasheet/docs described - U8g2 with the SH1106 driver is what actually
// works on this board.
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

// ---------------------------------------------------------------------
// SLIDING WINDOW BUFFERS
// ---------------------------------------------------------------------
float pressureBuffer[WINDOW_SIZE];
float accelMagBuffer[WINDOW_SIZE];
int bufferIndex = 0;
int samplesFilled = 0;

enum RiskLevel { RISK_LOW, RISK_MEDIUM, RISK_HIGH };
RiskLevel currentRisk = RISK_LOW;

unsigned long lastSampleTime = 0;

// ---------------------------------------------------------------------
// FORWARD DECLARATIONS
// ---------------------------------------------------------------------
void runCycle();
float readMPUAccelMagnitude();
void writeAPDSReg(uint8_t reg, uint8_t value);
uint16_t readAPDSClear();
float computePressureDrop();
float computeVariance(float* buf, int n);
RiskLevel computeRisk(float pressureDrop, float accelVariance, bool isIndoor);
void updateDisplay(float pressure, float drop, RiskLevel risk, bool isIndoor);
void soundBuzzer(RiskLevel risk);
void connectWiFi();
void logToCloud(float pressure, float temp, float accelVar, float drop, bool isIndoor, RiskLevel risk);

// ---------------------------------------------------------------------
// SETUP
// ---------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  Wire.begin();
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  // --- BMP180 ---
  if (!bmp.begin()) {
    Serial.println("BMP180 not found - check wiring.");
  }

  // --- MPU6050 (raw register wake-up) ---
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B); // PWR_MGMT_1
  Wire.write(0x00); // clear sleep bit
  byte mpuResult = Wire.endTransmission();
  if (mpuResult != 0) {
    Serial.println("MPU6050 not responding - check wiring.");
  }

  // --- APDS-9960 (raw register ALS setup) ---
  writeAPDSReg(APDS_REG_ATIME, 0xDB);   // ~103ms integration time
  writeAPDSReg(APDS_REG_CONTROL, 0x01); // 4x ALS gain
  writeAPDSReg(APDS_REG_ENABLE, 0x03);  // PON + AEN
  delay(50);

  // --- OLED (verified working: SH1106 via U8g2) ---
  u8g2.begin();
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.setCursor(0, 10);
  u8g2.print("PneumoGuard");
  u8g2.setCursor(0, 22);
  u8g2.print("Initializing...");
  u8g2.sendBuffer();

  connectWiFi();
}

// ---------------------------------------------------------------------
// MAIN LOOP
// ---------------------------------------------------------------------
void loop() {
  unsigned long now = millis();
  if (now - lastSampleTime >= SAMPLE_INTERVAL_MS || lastSampleTime == 0) {
    lastSampleTime = now;
    runCycle();
  }
}

// ---------------------------------------------------------------------
// STAGE 1 - DATA COLLECTION + STAGE 2 - SIGNAL PROCESSING
// ---------------------------------------------------------------------
void runCycle() {
  // --- BMP180 ---
  float pressure_hPa = bmp.readPressure() / 100.0;
  float temperature = bmp.readTemperature();

  // --- MPU6050 (raw accel read) ---
  float accelMag = readMPUAccelMagnitude();

  // --- APDS-9960 (raw ALS read) ---
  uint16_t clearLight = readAPDSClear();
  bool isIndoor = (clearLight < 500); // low ambient light -> indoor/night context; tune after testing in your actual room

  // --- Push into sliding window ---
  pressureBuffer[bufferIndex] = pressure_hPa;
  accelMagBuffer[bufferIndex] = accelMag;
  bufferIndex = (bufferIndex + 1) % WINDOW_SIZE;
  if (samplesFilled < WINDOW_SIZE) samplesFilled++;

  // --- Derived signals ---
  float pressureDrop = computePressureDrop();
  float accelVariance = computeVariance(accelMagBuffer, samplesFilled);

  Serial.printf("P=%.2f hPa  T=%.1fC  dropHPa=%.2f  accelVar=%.3f  light=%u  indoor=%d\n",
                pressure_hPa, temperature, pressureDrop, accelVariance, clearLight, isIndoor);

  // --- Stage 3: Risk scoring ---
  currentRisk = computeRisk(pressureDrop, accelVariance, isIndoor);

  // --- Stage 4: Alert & delivery ---
  updateDisplay(pressure_hPa, pressureDrop, currentRisk, isIndoor);
  soundBuzzer(currentRisk);
  logToCloud(pressure_hPa, temperature, accelVariance, pressureDrop, isIndoor, currentRisk);
}

// ---------------------------------------------------------------------
// RAW SENSOR READS
// ---------------------------------------------------------------------
float readMPUAccelMagnitude() {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B); // ACCEL_XOUT_H
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, (uint8_t)6);

  if (Wire.available() < 6) return 0.0;

  int16_t rawX = Wire.read() << 8 | Wire.read();
  int16_t rawY = Wire.read() << 8 | Wire.read();
  int16_t rawZ = Wire.read() << 8 | Wire.read();

  float ax = (rawX / MPU_LSB_PER_G) * G_TO_MS2;
  float ay = (rawY / MPU_LSB_PER_G) * G_TO_MS2;
  float az = (rawZ / MPU_LSB_PER_G) * G_TO_MS2;

  return sqrt(ax * ax + ay * ay + az * az);
}

void writeAPDSReg(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(APDS_ADDR);
  Wire.write(reg);
  Wire.write(value);
  Wire.endTransmission();
}

uint16_t readAPDSClear() {
  Wire.beginTransmission(APDS_ADDR);
  Wire.write(APDS_REG_CDATAL);
  Wire.endTransmission(false);
  Wire.requestFrom(APDS_ADDR, (uint8_t)2);

  if (Wire.available() < 2) return 0;
  return Wire.read() | (Wire.read() << 8);
}

// ---------------------------------------------------------------------
// SIGNAL PROCESSING HELPERS
// ---------------------------------------------------------------------
float computePressureDrop() {
  if (samplesFilled < 2) return 0.0;
  int newestIdx = (bufferIndex - 1 + WINDOW_SIZE) % WINDOW_SIZE;
  int oldestIdx = (bufferIndex - samplesFilled + WINDOW_SIZE) % WINDOW_SIZE;
  return pressureBuffer[oldestIdx] - pressureBuffer[newestIdx];
}

float computeVariance(float* buf, int n) {
  if (n < 2) return 0.0;
  float mean = 0;
  for (int i = 0; i < n; i++) mean += buf[i];
  mean /= n;
  float var = 0;
  for (int i = 0; i < n; i++) var += (buf[i] - mean) * (buf[i] - mean);
  return var / n;
}

// ---------------------------------------------------------------------
// STAGE 3 - RISK SCORING (weighted, context-adjusted)
// ---------------------------------------------------------------------
RiskLevel computeRisk(float pressureDrop, float accelVariance, bool isIndoor) {
  int score = 0;

  if (pressureDrop >= PRESSURE_DROP_HPA_HIGH) score += 2;
  else if (pressureDrop >= PRESSURE_DROP_HPA_MEDIUM) score += 1;

  if (accelVariance >= ACCEL_VAR_HIGH) score += 2;
  else if (accelVariance >= ACCEL_VAR_MEDIUM) score += 1;

  if (!isIndoor && score > 0) score += 1;

  if (score >= 4) return RISK_HIGH;
  if (score >= 2) return RISK_MEDIUM;
  return RISK_LOW;
}

// ---------------------------------------------------------------------
// STAGE 4a - OLED DISPLAY (not yet tested)
// ---------------------------------------------------------------------
void updateDisplay(float pressure, float drop, RiskLevel risk, bool isIndoor) {
  char line[24];

  u8g2.clearBuffer();

  // --- Top: small-font status lines ---
  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.setCursor(0, 9);
  u8g2.print("PneumoGuard");

  snprintf(line, sizeof(line), "P: %.1f hPa", pressure);
  u8g2.setCursor(0, 20);
  u8g2.print(line);

  snprintf(line, sizeof(line), "Drop: %.1f hPa", drop);
  u8g2.setCursor(0, 30);
  u8g2.print(line);

  snprintf(line, sizeof(line), "Ctx: %s", isIndoor ? "Indoor" : "Outdoor");
  u8g2.setCursor(0, 40);
  u8g2.print(line);

  u8g2.drawHLine(0, 43, 128);

  // --- Bottom: large-font risk level ---
  u8g2.setFont(u8g2_font_ncenB10_tr);
  u8g2.setCursor(0, 60);
  switch (risk) {
    case RISK_LOW:    u8g2.print("RISK: LOW");  break;
    case RISK_MEDIUM: u8g2.print("RISK: MED");  break;
    case RISK_HIGH:   u8g2.print("RISK: HIGH"); break;
  }

  u8g2.sendBuffer();
}

// ---------------------------------------------------------------------
// STAGE 4b - BUZZER ALERT (pin not yet confirmed)
// ---------------------------------------------------------------------
void soundBuzzer(RiskLevel risk) {
  switch (risk) {
    case RISK_LOW:
      digitalWrite(BUZZER_PIN, LOW);
      break;
    case RISK_MEDIUM:
      digitalWrite(BUZZER_PIN, HIGH);
      delay(150);
      digitalWrite(BUZZER_PIN, LOW);
      break;
    case RISK_HIGH:
      for (int i = 0; i < 3; i++) {
        digitalWrite(BUZZER_PIN, HIGH);
        delay(200);
        digitalWrite(BUZZER_PIN, LOW);
        delay(150);
      }
      break;
  }
}

// ---------------------------------------------------------------------
// STAGE 4c - CLOUD LOGGING (Wi-Fi) - not yet tested
// ---------------------------------------------------------------------
void connectWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  Serial.println(WiFi.status() == WL_CONNECTED ? " connected." : " failed (will retry logging silently).");
}

void logToCloud(float pressure, float temp, float accelVar, float drop, bool isIndoor, RiskLevel risk) {
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  http.begin(DASHBOARD_URL);
  http.addHeader("Content-Type", "application/json");

  StaticJsonDocument<256> doc;
  doc["pressure_hPa"] = pressure;
  doc["temperature_C"] = temp;
  doc["accel_variance"] = accelVar;
  doc["pressure_drop"] = drop;
  doc["context"] = isIndoor ? "indoor" : "outdoor";
  doc["risk"] = risk == RISK_HIGH ? "HIGH" : (risk == RISK_MEDIUM ? "MEDIUM" : "LOW");
  doc["timestamp"] = millis();

  String payload;
  serializeJson(doc, payload);

  int httpCode = http.POST(payload);

  if (httpCode == 200) {
    Serial.println("Cloud log: success.");
  } else if (httpCode == 302) {
    // Apps Script always replies to a POST with a redirect - the script has
    // already run and appended the row by this point. This is expected.
    Serial.println("Cloud log: success (302 is expected from Apps Script).");
  } else if (httpCode > 0) {
    Serial.printf("Cloud log: unexpected HTTP %d\n", httpCode);
  } else {
    Serial.printf("Cloud log: request failed (%s)\n", http.errorToString(httpCode).c_str());
  }

  http.end();
}
