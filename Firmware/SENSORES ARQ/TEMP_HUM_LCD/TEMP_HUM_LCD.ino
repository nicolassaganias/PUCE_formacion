#include <Wire.h>
#include <Adafruit_AHTX0.h>
#include <LiquidCrystal_I2C.h>

Adafruit_AHTX0 aht;
LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup() {
  Serial.begin(9600);
  Serial.println("AHT10 Sensor Test");

  Wire.begin();

  // Inicializar LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Iniciando...");

  // Inicializar AHT10
  if (!aht.begin()) {
    Serial.println("Failed to find AHT10 sensor!");
    lcd.setCursor(0, 1);
    lcd.print("AHT10 ERROR");
    while (1) delay(10);
  }

  Serial.println("AHT10 found and initialized.");
  lcd.setCursor(0, 1);
  lcd.print("AHT10 OK");
  delay(2000);
  lcd.clear();
}

void loop() {
  sensors_event_t humidity, temp;
  aht.getEvent(&humidity, &temp);

  // Serial
  Serial.print("Temperature: ");
  Serial.print(temp.temperature);
  Serial.println(" °C");

  Serial.print("Humidity: ");
  Serial.print(humidity.relative_humidity);
  Serial.println(" %");

  // LCD
  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  lcd.print(temp.temperature, 1);
  lcd.print(" C   ");

  lcd.setCursor(0, 1);
  lcd.print("Hum:  ");
  lcd.print(humidity.relative_humidity, 1);
  lcd.print(" %   ");

  delay(2000);
}
