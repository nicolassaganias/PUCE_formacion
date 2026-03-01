#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

#define SENSOR_PIN A0

// ====== Ajustes para TU módulo ======
#define V_CLEAR   3.70   // voltaje agua clara
#define V_DIRTY   1.30   // voltaje agua muy turbia
#define NTU_MAX   2000   // NTU máximo definido

float volt;
float ntu;

void setup()
{
  Serial.begin(9600);
  lcd.init();
  lcd.backlight();
}

void loop()
{
  // -------- Promedio para reducir ruido --------
  volt = 0.0;
  for (int i = 0; i < 500; i++) {
    volt += analogRead(SENSOR_PIN) * (5.0 / 1023.0);
  }
  volt /= 500.0;

  // -------- Cálculo NTU recalibrado --------
  if (volt >= V_CLEAR) {
    ntu = 0;
  }
  else if (volt <= V_DIRTY) {
    ntu = NTU_MAX;
  }
  else {
    ntu = (V_CLEAR - volt) * NTU_MAX / (V_CLEAR - V_DIRTY);
  }

  // -------- Mostrar en LCD --------
  lcd.setCursor(0, 0);
  lcd.print("V:");
  lcd.print(volt, 2);
  lcd.print("     ");

  lcd.setCursor(0, 1);
  lcd.print("NTU:");
  lcd.print((int)ntu);
  lcd.print("     ");

  // -------- Clasificacion por umbrales --------
  lcd.setCursor(9, 1);
  if (ntu < 50) {
    lcd.print("CLEAR ");
  }
  else if (ntu < 500) {
    lcd.print("CLOUDY");
  }
  else {
    lcd.print("DIRTY ");
  }

  // -------- Debug serie --------
  Serial.print("Voltaje: ");
  Serial.print(volt, 3);
  Serial.print(" V  | NTU: ");
  Serial.println(ntu);

  delay(1000);
}
