#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <RTClib.h>
#include <SPI.h>
#include <SD.h>

#define SD_CS 10

LiquidCrystal_I2C lcd(0x27, 16, 2);   // Cambiar a 0x3F o 0x27 si corresponde.
RTC_DS3231 rtc;

void setup() {
  Serial.begin(9600);

  // LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.print("Iniciando...");

  // RTC
  if (!rtc.begin()) {
    lcd.setCursor(0, 1);
    lcd.print("RTC ERROR");
    while (1);
  }

  if (rtc.lostPower()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  // SD
  if (!SD.begin(SD_CS)) {
    lcd.clear();
    lcd.print("SD ERROR");
    while (1);
  }

  // Crear CSV con encabezado si no existe
  if (!SD.exists("datos.csv")) {
    File f = SD.open("datos.csv", FILE_WRITE);
    if (f) {
      f.println("fecha,hora");
      f.close();
    }
  }

  lcd.clear();
  lcd.print("SD RTC OK");
  delay(2000);
}

void loop() {
  DateTime now = rtc.now();

  // ----- LCD -----
  lcd.clear();

  // Línea 1: fecha (dd/mm/yy)
  lcd.setCursor(0, 0);
  if (now.day() < 10) lcd.print("0");
  lcd.print(now.day());
  lcd.print("/");
  if (now.month() < 10) lcd.print("0");
  lcd.print(now.month());
  lcd.print("/");
  if ((now.year() % 100) < 10) lcd.print("0");
  lcd.print(now.year() % 100);

  // Línea 2: hora (hh:mm:ss)
  lcd.setCursor(0, 1);
  if (now.hour() < 10) lcd.print("0");
  lcd.print(now.hour());
  lcd.print(":");
  if (now.minute() < 10) lcd.print("0");
  lcd.print(now.minute());
  lcd.print(":");
  if (now.second() < 10) lcd.print("0");
  lcd.print(now.second());

  // ----- SD (cada 1 segundo) -----
  File f = SD.open("datos.csv", FILE_WRITE);
  if (f) {
    // fecha dd/mm/yy
    if (now.day() < 10) f.print("0");
    f.print(now.day());
    f.print("/");
    if (now.month() < 10) f.print("0");
    f.print(now.month());
    f.print("/");
    if ((now.year() % 100) < 10) f.print("0");
    f.print(now.year() % 100);
    f.print(",");

    // hora hh:mm:ss
    if (now.hour() < 10) f.print("0");
    f.print(now.hour());
    f.print(":");
    if (now.minute() < 10) f.print("0");
    f.print(now.minute());
    f.print(":");
    if (now.second() < 10) f.print("0");
    f.println(now.second());

    f.close();
  }

  delay(1000);
}
