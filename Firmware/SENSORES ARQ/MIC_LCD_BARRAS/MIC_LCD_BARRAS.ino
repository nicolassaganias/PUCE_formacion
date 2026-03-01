/*
  EJEMPLO SIMPLE
  Microfono amplificado en A2 + LCD I2C 0x27
  Muestra: BAJO / MEDIO / ALTO + barra proporcional a TH_LOW y TH_HIGH

  SIN calibracion automatica
  SIN CSV
  SOLO ejemplo visual basico

  Requisitos:
  - Libreria LiquidCrystal_I2C
  - LCD 16x2 I2C direccion 0x27
*/

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

const uint8_t MIC_PIN = A2;

// Ventana de muestreo (ms)
const uint16_t WINDOW_MS = 60;

// ===== AJUSTAR SEGUN TU AMBIENTE =====
// Ejemplo si ruido base ~90
int TH_LOW  = 110;   // debajo = BAJO
int TH_HIGH = 180;   // arriba = ALTO

// Suavizado (0..1)
float smoothP2P = 0.0;
const float ALPHA = 0.2;

// ===== FUNCIONES =====

int readPeakToPeak(uint16_t windowMs) {
  uint32_t start = millis();
  int sMin = 1023;
  int sMax = 0;

  while (millis() - start < windowMs) {
    int s = analogRead(MIC_PIN);
    if (s < sMin) sMin = s;
    if (s > sMax) sMax = s;
  }
  return sMax - sMin;
}

const char* levelLabel(float p2p) {
  if (p2p < TH_LOW)  return "BAJO ";
  if (p2p < TH_HIGH) return "MEDIO";
  return "ALTO ";
}

// Barra alineada con TH_LOW y TH_HIGH
uint8_t barCount(float p2p) {

  float minV = (float)TH_LOW;
  float maxV = (float)TH_HIGH;

  float x = (p2p - minV) / (maxV - minV);

  if (x < 0) x = 0;
  if (x > 1) x = 1;

  return (uint8_t)(x * 16.0 + 0.5);
}

// ===== SETUP =====

void setup() {

  lcd.init();
  lcd.backlight();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Mic A2 + LCD");
  lcd.setCursor(0, 1);
  lcd.print("Iniciando...");
  delay(1000);

  lcd.clear();
}

// ===== LOOP =====

void loop() {

  int p2p = readPeakToPeak(WINDOW_MS);

  // suavizado
  smoothP2P = (1.0 - ALPHA) * smoothP2P + ALPHA * p2p;

  const char* lvl = levelLabel(smoothP2P);
  uint8_t bars = barCount(smoothP2P);

  // ---- LINEA 1 ----
  lcd.setCursor(0, 0);
  lcd.print("Nivel: ");
  lcd.print(lvl);
  lcd.print("   ");

  // valor numerico (solo referencia visual)
  lcd.setCursor(12, 0);
  if (smoothP2P < 100) lcd.print(" ");
  if (smoothP2P < 10) lcd.print(" ");
  lcd.print((int)smoothP2P);

  // ---- LINEA 2 BARRA ----
  lcd.setCursor(0, 1);
  for (int i = 0; i < 16; i++) {
    if (i < bars) lcd.print((char)255);
    else lcd.print(" ");
  }

  delay(120);
}
