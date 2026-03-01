#include <Wire.h>
#include <BH1750.h>
#include <LiquidCrystal_I2C.h>

BH1750 lightMeter;
LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup() {
  Serial.begin(9600);
  Wire.begin();

  // LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Iniciando...");

  // BH1750
  if (!lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
    Serial.println("Error BH1750");
    lcd.setCursor(0, 1);
    lcd.print("BH1750 ERROR");
    while (1);
  }

  Serial.println("BH1750 OK");
  lcd.setCursor(0, 1);
  lcd.print("BH1750 OK");
  delay(2000);
  lcd.clear();
}

void loop() {
  float lux = lightMeter.readLightLevel();

  // Serial
  Serial.print("Luz: ");
  Serial.print(lux);
  Serial.println(" lx");

  // LCD
  lcd.setCursor(0, 0);
  lcd.print("Luz:");
  lcd.setCursor(5, 0);
  lcd.print(lux, 1);
  lcd.print(" lx   ");

  lcd.setCursor(0, 1);
  lcd.print("                "); // limpiar linea

  delay(1000);
}
