/*
UNO - SIMPLE (2 timers) SIN DO + páginas LCD (1 parámetro por página)
Esquina superior derecha (cols 11-15) SOLO fecha dd/mm.

Pines:
- DS18B20: D2
- Turbidez: A0
- TDS: A1
- SD CS: D10
- LCD I2C: 0x27
- RTC DS3231: I2C

LCD:
Linea 0: <param>........dd/mm   (fecha fija a la derecha)
Linea 1: (vacío o unidad)

Rota páginas:
0) Temp
1) Turb
2) TDS

Timers:
- LCD_INTERVAL: lee sensores + rota/actualiza LCD
- LOG_INTERVAL: guarda CSV
*/

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <RTClib.h>
#include <SPI.h>
#include <SD.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <math.h>

// ---------- Pines ----------
#define PIN_TEMP 2
#define PIN_TURBIDEZ A0
#define PIN_TDS A1
#define SD_CS 10

// ---------- Objetos ----------
LiquidCrystal_I2C lcd(0x27, 16, 2);
RTC_DS3231 rtc;
File logFile;

OneWire oneWire(PIN_TEMP);
DallasTemperature sensors(&oneWire);

// ---------- Timers (solo 2) ----------
unsigned long lastLcd = 0;
unsigned long lastLog = 0;

const unsigned long LCD_INTERVAL = 3000;   // refresco LCD + lectura sensores
const unsigned long LOG_INTERVAL = 5000;  // guardado SD

// ---------- Estado ----------
bool sdOK = false;
bool rtcOK = false;

float temperatura = NAN;
float turbidezNTU = NAN;
float tdsPpm = NAN;

uint8_t page = 0;

// ---------- Funciones ----------
static void printFechaLCD() {
  if (!rtcOK) return;
  DateTime now = rtc.now();
  char fecha[6];
  sprintf(fecha, "%02d/%02d", now.day(), now.month());
  lcd.setCursor(11, 0);  // cols 11..15
  lcd.print(fecha);
}

// === TURBIDEZ EN NTU (tu método) ===
float leerTurbidezNTU() {
  int raw = analogRead(PIN_TURBIDEZ);
  float volt = raw * (5.0 / 1023.0);

  float ntu = -1120.4 * volt * volt
              + 5742.3 * volt
              - 4352.9;

  if (ntu < 0) ntu = 0;
  return ntu;
}

// === TDS simple (placeholder) ===
float leerTdsPlaceholder() {
  int raw = analogRead(PIN_TDS);
  float v = raw * (5.0 / 1023.0);
  float ppm = v * 500.0;  // 0..2500 aprox para visualizar
  if (ppm < 0) ppm = 0;
  return ppm;
}

static void ensureCsvHeader() {
  if (!SD.exists("h2o.csv")) {
    logFile = SD.open("h2o.csv", FILE_WRITE);
    if (logFile) {
      logFile.println(F("fecha,hora,temperatura_C,turbidez_NTU,tds_ppm"));
      logFile.close();
    }
  }
}

// Lee todo (para LCD y para LOG)
static void leerSensores() {
  sensors.requestTemperatures();
  temperatura = sensors.getTempCByIndex(0);

  turbidezNTU = leerTurbidezNTU();
  tdsPpm = leerTdsPlaceholder();
}

static void clearLine(uint8_t row) {
  lcd.setCursor(0, row);
  lcd.print(F("                "));  // 16 espacios
}

// Actualiza LCD según página (1 parámetro por página, centrado/grande)
static void actualizarLCD() {
  // Limpiar ambas líneas
  clearLine(0);
  clearLine(1);

  // Fecha siempre arriba a la derecha
  printFechaLCD();

  if (page == 0) {
    // Temp
    lcd.setCursor(0, 0);
    lcd.print(F("Temp:"));
    lcd.print(temperatura, 1);
    lcd.print((char)223);
    lcd.print(F("C"));
  } 
  else if (page == 1) {
    // Turbidez
    lcd.setCursor(0, 0);
    lcd.print(F("Turb:"));
    lcd.print(turbidezNTU, 0);
    lcd.print(F(" NTU"));
  } 
  else {
    // TDS
    lcd.setCursor(0, 0);
    lcd.print(F("TDS:"));
    lcd.print(tdsPpm, 0);
    lcd.print(F(" ppm"));
  }

  // Rotar página (3 páginas ahora: 0,1,2)
  page = (page + 1) % 3;
}

static void logSD() {
  if (!(sdOK && rtcOK)) return;

  DateTime t = rtc.now();
  logFile = SD.open("h2o.csv", FILE_WRITE);
  if (!logFile) return;

  logFile.print(t.day());
  logFile.print("/");
  logFile.print(t.month());
  logFile.print("/");
  logFile.print(t.year());
  logFile.print(",");

  logFile.print(t.hour());
  logFile.print(":");
  logFile.print(t.minute());
  logFile.print(":");
  logFile.print(t.second());
  logFile.print(",");

  logFile.print(temperatura, 2);
  logFile.print(",");
  logFile.print(turbidezNTU, 1);
  logFile.print(",");
  logFile.println(tdsPpm, 0);

  logFile.close();
  Serial.println(F("[SD] Datos guardados"));
}

// ---------- SETUP ----------
void setup() {
  Serial.begin(9600);

  lcd.init();
  lcd.backlight();

  sensors.begin();
  pinMode(PIN_TURBIDEZ, INPUT);
  pinMode(PIN_TDS, INPUT);

  rtcOK = rtc.begin();
  if (!rtcOK) Serial.println(F("RTC no detectado"));

  pinMode(10, OUTPUT);
  digitalWrite(10, HIGH);

  sdOK = SD.begin(SD_CS);
  if (!sdOK) {
    Serial.println(F("SD no detectada"));
  } else {
    ensureCsvHeader();
  }

  Serial.println(F("Kit H2O (3 params + fecha fija)"));

  // primera lectura
  leerSensores();
  actualizarLCD();
}

// ---------- LOOP ----------
void loop() {
  unsigned long nowMs = millis();

  if (nowMs - lastLcd >= LCD_INTERVAL) {
    lastLcd = nowMs;
    leerSensores();
    actualizarLCD();
  }

  if (nowMs - lastLog >= LOG_INTERVAL) {
    lastLog = nowMs;
    logSD();
  }
}