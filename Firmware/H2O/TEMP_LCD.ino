#include <OneWire.h>                
#include <DallasTemperature.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

OneWire ourWire(2);                
DallasTemperature sensors(&ourWire);

// Dirección 0x27, 16 columnas, 2 filas
LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup() {
  delay(1000);
  Serial.begin(9600);
  sensors.begin();

  lcd.init();          // Inicializa la LCD
  lcd.backlight();     // Enciende la luz de fondo
  lcd.setCursor(0,0);
  lcd.print("Iniciando...");
  delay(2000);
  lcd.clear();
}

void loop() {
  sensors.requestTemperatures();
  float temp = sensors.getTempCByIndex(0);

  // Serial Monitor
  Serial.print("Temperatura= ");
  Serial.print(temp);
  Serial.println(" C");

  // LCD
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Temperatura:");
  lcd.setCursor(0,1);
  lcd.print(temp);
  lcd.print(" C");

  delay(1000);
}
