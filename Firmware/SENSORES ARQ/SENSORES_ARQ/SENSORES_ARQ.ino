#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <RTClib.h>
#include <SPI.h>
#include <SD.h>

// ---------- Pines ----------
#define PIN_MIC A2
#define PIN_PIR 3
#define PIN_MQ7 A0
#define PIN_MQ2 A1
#define SD_CS 10

// ---------- Objetos ----------
LiquidCrystal_I2C lcd(0x27, 16, 2);  // 0x27 o 0x3F
RTC_DS3231 rtc;
File logFile;

// ---------- Timers ----------
unsigned long lastScreenChange = 0;
unsigned long lastLog = 0;
unsigned long lastSerial = 0;

const unsigned long SCREEN_INTERVAL = 4000;
const unsigned long LOG_INTERVAL = 6000;
const unsigned long SERIAL_INTERVAL = 1000;

byte screen = 0;
bool sdOK = false;
bool rtcOK = false;

// ---------- Funciones ----------
String nivel(int v, int bajo, int medio) {
  if (v < bajo) return "BAJO";
  if (v < medio) return "MEDIO";
  return "ALTO";
}

int nivelSonido() {
  int minVal = 1023;
  int maxVal = 0;
  unsigned long start = millis();

  while (millis() - start < 200) {
    int v = analogRead(PIN_MIC);
    if (v < minVal) minVal = v;
    if (v > maxVal) maxVal = v;
  }
  return maxVal - minVal;
}

void printFechaHoraLCD() {
  if (!rtcOK) return;
  DateTime now = rtc.now();

  char buf[9];  // dd/mm hh
  sprintf(buf, "%02d/%02d %02d", now.day(), now.month(), now.hour());
  lcd.setCursor(11, 0);
  lcd.print(buf);
}

// ---------- SETUP ----------
void setup() {
  pinMode(PIN_PIR, INPUT);
  Serial.begin(9600);
  Serial.println(F("=== INICIO SETUP ==="));

  lcd.init();
  lcd.backlight();
  Serial.println(F("[LCD] OK"));

  rtcOK = rtc.begin();
  if (rtcOK) {
    Serial.println(F("[RTC] Detectado"));
  } else {
    Serial.println(F("[RTC] NO detectado"));
  }

  sdOK = SD.begin(SD_CS);
  if (sdOK) {
    Serial.println(F("[SD] Detectada"));
    if (!SD.exists("datos.csv")) {
      logFile = SD.open("datos.csv", FILE_WRITE);
      logFile.println("fecha,hora,mic_amp,pir,mq7,mq2");
      logFile.close();
      Serial.println(F("[SD] Archivo creado"));
    }
  } else {
    Serial.println(F("[SD] NO detectada"));
  }

  Serial.println(F("=== FIN SETUP ==="));
}

// ---------- LOOP ----------
void loop() {
  unsigned long nowMs = millis();

  int micAmp = nivelSonido();
  int pir = digitalRead(PIN_PIR);
  int mq7 = analogRead(PIN_MQ7);
  int mq2 = analogRead(PIN_MQ2);

  // ---- Cambio de pantalla ----
  if (nowMs - lastScreenChange >= SCREEN_INTERVAL) {
    lastScreenChange = nowMs;
    screen = (screen + 1) % 2;
    lcd.clear();
  }

  // ---- PANTALLAS ----
  if (screen == 0) {
    lcd.setCursor(0, 0);
    lcd.print("PIR:");
    lcd.print(pir ? "GENTE" : "VACIO");

    lcd.setCursor(0, 1);
    lcd.print("MIC:");
    lcd.print(nivel(micAmp, 40, 150));
  }

  if (screen == 1) {
    lcd.setCursor(0, 0);
    lcd.print("MQ2:");
    lcd.print(nivel(mq2, 300, 600));

    lcd.setCursor(0, 1);
    lcd.print("MQ7:");
    lcd.print(nivel(mq7, 300, 600));
  }

  printFechaHoraLCD();

  // ---- DEBUG SERIAL ----
  if (nowMs - lastSerial >= SERIAL_INTERVAL) {
    lastSerial = nowMs;

    if (rtcOK) {
      DateTime t = rtc.now();
      Serial.print("[");
      Serial.print(t.day());
      Serial.print("/");
      Serial.print(t.month());
      Serial.print(" ");
      Serial.print(t.hour());
      Serial.print(":");
      Serial.print(t.minute());
      Serial.print(":");
      Serial.print(t.second());
      Serial.print("] ");
    }

    Serial.print("MIC=");
    Serial.print(micAmp);
    Serial.print(" ADC ");

    Serial.print("| PIR=");
    Serial.print(pir ? "DETECCION" : "SIN_MOV");

    Serial.print(" | MQ2=");
    Serial.print(mq2);
    Serial.print(" ADC ");

    Serial.print("| MQ7=");
    Serial.print(mq7);
    Serial.println(" ADC");
  }

  // ---- LOG EN SD ----
  if (sdOK && rtcOK && nowMs - lastLog >= LOG_INTERVAL) {
    lastLog = nowMs;
    DateTime t = rtc.now();

    logFile = SD.open("datos.csv", FILE_WRITE);
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

    logFile.print(micAmp);
    logFile.print(",");
    logFile.print(pir);
    logFile.print(",");
    logFile.print(mq7);
    logFile.print(",");
    logFile.println(mq2);
    logFile.close();

    Serial.println("[SD] Datos guardados");
  }
}
