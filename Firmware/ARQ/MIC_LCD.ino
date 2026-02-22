/*
  VERSION ULTRA SIMPLE
  Microfono en A2 + LCD I2C 0x27
  Solo muestra: BAJO / MEDIO / ALTO
*/

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

const int MIC_PIN = A2;
const int WINDOW_MS = 60;

// Ajustar segun tu ambiente
int TH_LOW  = 110;
int TH_HIGH = 180;

int readPeakToPeak(int windowMs) {
  unsigned long start = millis();
  int minVal = 1023;
  int maxVal = 0;

  while (millis() - start < windowMs) {
    int val = analogRead(MIC_PIN);
    if (val < minVal) minVal = val;
    if (val > maxVal) maxVal = val;
  }
  return maxVal - minVal;
}

const char* getLevel(int p2p) {
  if (p2p < TH_LOW) return "BAJO";
  if (p2p < TH_HIGH) return "MEDIO";
  return "ALTO";
}

void setup() {
  lcd.init();
  lcd.backlight();

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Medidor Sonido");
  delay(1000);
  lcd.clear();
}

void loop() {

  int p2p = readPeakToPeak(WINDOW_MS);
  const char* nivel = getLevel(p2p);

  lcd.setCursor(0,0);
  lcd.print("Nivel Sonido:");

  lcd.setCursor(0,1);
  lcd.print("                "); // limpia linea
  lcd.setCursor(0,1);
  lcd.print(nivel);

  delay(200);
}
