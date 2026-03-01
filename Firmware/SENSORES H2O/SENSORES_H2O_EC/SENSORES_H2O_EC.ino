/*
UNO memory-safe - SIN DO (con EC)
Incluye: LCD I2C 0x27 + RTC DS3231 + SD SPI + DS18B20 + pH Surveyor + EC Atlas UART + TDS analógico + Turbidez
CSV: Fecha,Hora,TempC,pH,EC,TDS_EC,SAL,GRAV,TDS_ppm,Turb_V,Turb_NTU

Wiring UNO:
I2C: SDA A4 / SCL A5 (LCD + RTC)
SD SPI: CS D10, MOSI D11, MISO D12, SCK D13
DS18B20: D4
EC UART: RX D2 (desde TX del EC) / TX D3 (hacia RX del EC)
pH: A0
TDS: A2
Turb: A3
*/

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <SD.h>
#include <RTClib.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <SoftwareSerial.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

// ---------------- CONFIG ----------------
const unsigned long LOG_INTERVAL_MS = 30000;
const unsigned long LCD_INTERVAL_MS = 3000;

const uint8_t ONE_WIRE_PIN = 4;

#define PH_PIN   A0
#define TDS_PIN  A2
#define TURB_PIN A3

const uint8_t SD_CS_PIN = 10;
const char CSV_NAME[] = "datos.csv";

// EC UART
#define EC_RX 2
#define EC_TX 3

// TDS
#define VREF 5.0
#define SCOUNT 20

// Turbidez
#define V_CLEAR   3.70
#define V_DIRTY   1.30
#define NTU_MAX   2000

// ---------------- OBJETOS ----------------
LiquidCrystal_I2C lcd(0x27, 16, 2);
RTC_DS3231 rtc;

OneWire oneWire(ONE_WIRE_PIN);
DallasTemperature tempSensor(&oneWire);

SoftwareSerial ecSerial(EC_RX, EC_TX);

// Surveyor pH
// #define USE_PULSE_OUT

#ifdef USE_PULSE_OUT
  #include "ph_iso_surveyor.h"
  Surveyor_pH_Isolated pH = Surveyor_pH_Isolated(PH_PIN);
#else
  #include "ph_surveyor.h"
  Surveyor_pH pH = Surveyor_pH(PH_PIN);
#endif

// ---------------- VARIABLES ----------------
unsigned long lastLogMs = 0;
unsigned long lastLcdMs = 0;
uint8_t lcdPage = 0;

float tempC = NAN;
float phVal = NAN;

// EC parsed: EC,TDS,SAL,GRAV
float ecVal  = NAN;
float ecTds  = NAN;
float ecSal  = NAN;
float ecGrav = NAN;

// TDS analógico
uint16_t tdsBuf[SCOUNT];
uint16_t tdsBufTmp[SCOUNT];
uint8_t tdsIdx = 0;
float tdsPpm = NAN;

// Turbidez
float turbVolt = NAN;
float turbNTU  = NAN;

// Serial command buffer
const uint8_t CMD_BUFLEN = 32;
char user_data[CMD_BUFLEN];

// EC line buffer
char ecLine[32];

// ---------------- UTIL ----------------
static void print2(File &f, int v) { if (v < 10) f.print('0'); f.print(v); }
static void print2s(Stream &s, int v) { if (v < 10) s.print('0'); s.print(v); }

static int median_u16(uint16_t a[], uint8_t n) {
  for (uint8_t i = 0; i < n; i++) tdsBufTmp[i] = a[i];
  for (uint8_t j = 0; j < n - 1; j++) {
    for (uint8_t i = 0; i < n - j - 1; i++) {
      if (tdsBufTmp[i] > tdsBufTmp[i + 1]) {
        uint16_t t = tdsBufTmp[i];
        tdsBufTmp[i] = tdsBufTmp[i + 1];
        tdsBufTmp[i + 1] = t;
      }
    }
  }
  if (n & 1) return tdsBufTmp[(n - 1) / 2];
  return (tdsBufTmp[n / 2] + tdsBufTmp[n / 2 - 1]) / 2;
}

static void ensureCsvHeader() {
  if (!SD.exists(CSV_NAME)) {
    File f = SD.open(CSV_NAME, FILE_WRITE);
    if (f) {
      f.println(F("Fecha,Hora,TempC,pH,EC,TDS_EC,SAL,GRAV,TDS_ppm,Turb_V,Turb_NTU"));
      f.close();
    }
  }
}

// ---------------- Comandos pH/EC ----------------
// Serial:
// PH,CAL,7 | PH,CAL,4 | PH,CAL,10 | PH,CAL,CLEAR
// EC,<comando_atlas>  (ej: EC,R)

static void parse_cmd_PH(char* s) {
  strupr(s);
  if (strcmp(s, "CAL,7") == 0) { pH.cal_mid();  Serial.println(F("PH MID CALIBRATED")); }
  else if (strcmp(s, "CAL,4") == 0) { pH.cal_low();  Serial.println(F("PH LOW CALIBRATED")); }
  else if (strcmp(s, "CAL,10") == 0) { pH.cal_high(); Serial.println(F("PH HIGH CALIBRATED")); }
  else if (strcmp(s, "CAL,CLEAR") == 0) { pH.cal_clear(); Serial.println(F("PH CALIBRATION CLEARED")); }
}

static void ec_send_command(const char* cmd) {
  ecSerial.print(cmd);
  ecSerial.print('\r');
}

static void handleUserCommand(char* line) {
  for (uint8_t i = 0; line[i]; i++) {
    if (line[i] == '\r' || line[i] == '\n') line[i] = 0;
  }

  if (strncmp(line, "PH,", 3) == 0) { parse_cmd_PH(line + 3); return; }

  if (strncmp(line, "EC,", 3) == 0) {
    ec_send_command(line + 3);
    Serial.println(F("EC CMD SENT"));
    return;
  }

  Serial.println(F("CMD? Use PH,... or EC,..."));
}

// ---------------- Lecturas ----------------
static void readTemp() {
  tempSensor.requestTemperatures();
  tempC = tempSensor.getTempCByIndex(0);
}

static void readPH() {
  phVal = pH.read_ph();
}

static void sampleTDSBuffer() {
  tdsBuf[tdsIdx] = (uint16_t)analogRead(TDS_PIN);
  tdsIdx++;
  if (tdsIdx >= SCOUNT) tdsIdx = 0;
}

static void computeTDS() {
  float averageVoltage = median_u16(tdsBuf, SCOUNT) * (VREF / 1024.0);

  float temperature = (isnan(tempC) ? 25.0 : tempC);
  float compensationCoefficient = 1.0 + 0.02 * (temperature - 25.0);
  float compensationVoltage = averageVoltage / compensationCoefficient;

  tdsPpm = (133.42 * compensationVoltage * compensationVoltage * compensationVoltage
          - 255.86 * compensationVoltage * compensationVoltage
          + 857.39 * compensationVoltage) * 0.5;
}

static void readTurbidity() {
  float v = 0.0;
  for (int i = 0; i < 300; i++) {
    v += analogRead(TURB_PIN) * (5.0 / 1023.0);
  }
  v /= 300.0;
  turbVolt = v;

  if (turbVolt >= V_CLEAR) turbNTU = 0;
  else if (turbVolt <= V_DIRTY) turbNTU = NTU_MAX;
  else turbNTU = (V_CLEAR - turbVolt) * NTU_MAX / (V_CLEAR - V_DIRTY);
}

// EC reading: pide "R" y parsea "EC,TDS,SAL,GRAV"
static bool ec_read_line(char* out, uint8_t outLen, unsigned long timeoutMs) {
  uint8_t idx = 0;
  unsigned long t0 = millis();

  while (millis() - t0 < timeoutMs) {
    while (ecSerial.available() > 0) {
      char c = (char)ecSerial.read();
      if (c == '\r') {
        out[idx] = 0;
        return (idx > 0);
      }
      if (idx < outLen - 1) out[idx++] = c;
    }
  }
  out[idx] = 0;
  return false;
}

static void ec_parse_csv(char* line) {
  // Espera: EC,TDS,SAL,GRAV
  char* EC   = strtok(line, ",");
  char* TDS  = strtok(NULL, ",");
  char* SAL  = strtok(NULL, ",");
  char* GRAV = strtok(NULL, ",");

  if (EC)   ecVal  = atof(EC);
  if (TDS)  ecTds  = atof(TDS);
  if (SAL)  ecSal  = atof(SAL);
  if (GRAV) ecGrav = atof(GRAV);
}

static void readEC() {
  ec_send_command("R");
  if (ec_read_line(ecLine, sizeof(ecLine), 800)) {
    if (isdigit((unsigned char)ecLine[0])) {
      ec_parse_csv(ecLine);
    }
  }
}

// ---------------- LCD ----------------
static void lcdPrintFloat(float v, uint8_t dec) {
  if (isnan(v)) lcd.print(F("NA"));
  else lcd.print(v, dec);
}

static void updateLCD() {
  lcd.clear();
  switch (lcdPage) {
    case 0:
      lcd.setCursor(0,0); lcd.print(F("T:")); lcdPrintFloat(tempC,1); lcd.print(F("C"));
      lcd.setCursor(0,1); lcd.print(F("pH:")); lcdPrintFloat(phVal,2);
      break;
    case 1:
      lcd.setCursor(0,0); lcd.print(F("EC:")); lcdPrintFloat(ecVal,2);
      lcd.setCursor(0,1); lcd.print(F("TDS:")); lcdPrintFloat(ecTds,0); lcd.print(F("ppm"));
      break;
    case 2:
      lcd.setCursor(0,0); lcd.print(F("SAL:")); lcdPrintFloat(ecSal,2);
      lcd.setCursor(0,1); lcd.print(F("GR:")); lcdPrintFloat(ecGrav,3);
      break;
    case 3:
      lcd.setCursor(0,0); lcd.print(F("TDS ana:"));
      lcd.setCursor(0,1); lcdPrintFloat(tdsPpm,0); lcd.print(F(" ppm"));
      break;
    default:
      lcd.setCursor(0,0); lcd.print(F("V:")); lcdPrintFloat(turbVolt,2);
      lcd.setCursor(0,1); lcd.print(F("NTU:")); lcdPrintFloat(turbNTU,0);
      break;
  }
  lcdPage = (lcdPage + 1) % 5;
}

// ---------------- SD log ----------------
static void logToSD(const DateTime &now) {
  File f = SD.open(CSV_NAME, FILE_WRITE);
  if (!f) return;

  f.print(now.year()); f.print('-'); print2(f, now.month()); f.print('-'); print2(f, now.day());
  f.print(',');

  print2(f, now.hour()); f.print(':'); print2(f, now.minute()); f.print(':'); print2(f, now.second());
  f.print(',');

  f.print(tempC); f.print(',');
  f.print(phVal); f.print(',');

  f.print(ecVal);  f.print(',');
  f.print(ecTds);  f.print(',');
  f.print(ecSal);  f.print(',');
  f.print(ecGrav); f.print(',');

  f.print(tdsPpm); f.print(',');
  f.print(turbVolt); f.print(',');
  f.println(turbNTU);

  f.close();
}

// ---------------- SETUP/LOOP ----------------
void setup() {
  Serial.begin(9600);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print(F("Iniciando..."));
  delay(500);

  if (!rtc.begin()) {
    lcd.clear(); lcd.print(F("RTC ERROR"));
    while (1);
  }

  if (!SD.begin(SD_CS_PIN)) {
    lcd.clear(); lcd.print(F("SD ERROR"));
    while (1);
  }
  ensureCsvHeader();

  tempSensor.begin();
  pH.begin();

  ecSerial.begin(9600);

  for (uint8_t i = 0; i < SCOUNT; i++) {
    tdsBuf[i] = (uint16_t)analogRead(TDS_PIN);
    delay(5);
  }

  lcd.clear();
  lcd.print(F("Listo"));
  delay(500);
  lcd.clear();
}

void loop() {
  // TDS sampling continuo
  static unsigned long lastTdsSample = 0;
  if (millis() - lastTdsSample >= 40) {
    lastTdsSample = millis();
    sampleTDSBuffer();
  }

  // comandos
  if (Serial.available() > 0) {
    uint8_t n = Serial.readBytesUntil(13, user_data, sizeof(user_data) - 1);
    user_data[n] = 0;
    if (n) handleUserCommand(user_data);
  }

  // LCD
  if (millis() - lastLcdMs >= LCD_INTERVAL_MS) {
    lastLcdMs = millis();
    updateLCD();
  }

  // LOG
  if (millis() - lastLogMs >= LOG_INTERVAL_MS) {
    lastLogMs = millis();

    readTemp();
    readPH();
    readEC();
    computeTDS();
    readTurbidity();

    DateTime now = rtc.now();
    logToSD(now);

    Serial.print(F("LOG "));
    Serial.print(now.year()); Serial.print('-'); print2s(Serial, now.month()); Serial.print('-'); print2s(Serial, now.day());
    Serial.print(' ');
    print2s(Serial, now.hour()); Serial.print(':'); print2s(Serial, now.minute()); Serial.print(':'); print2s(Serial, now.second());
    Serial.println();
  }
}
