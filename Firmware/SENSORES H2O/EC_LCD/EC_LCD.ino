#include <SoftwareSerial.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define rx 2
#define tx 3

SoftwareSerial myserial(rx, tx);

// LCD 16x2 dirección 0x27
LiquidCrystal_I2C lcd(0x27, 16, 2);

String inputstring = "";
String sensorstring = "";
boolean sensor_string_complete = false;

void setup() {
  Serial.begin(9600);
  myserial.begin(9600);

  inputstring.reserve(10);
  sensorstring.reserve(30);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("EC Monitor");
  delay(1500);
  lcd.clear();
}

void loop() {

  if (Serial.available()) {
    inputstring = Serial.readStringUntil(13);
    myserial.print(inputstring);
    myserial.print('\r');
    inputstring = "";
  }

  if (myserial.available() > 0) {
    char inchar = (char)myserial.read();
    sensorstring += inchar;
    if (inchar == '\r') {
      sensor_string_complete = true;
    }
  }

  if (sensor_string_complete == true) {
    if (isdigit(sensorstring[0]) == false) {
      Serial.println(sensorstring);
    }
    else {
      print_EC_data();
    }
    sensorstring = "";
    sensor_string_complete = false;
  }
}

void print_EC_data(void) {

  char sensorstring_array[30];
  char *EC;
  char *TDS;
  char *SAL;
  char *GRAV;

  sensorstring.toCharArray(sensorstring_array, 30);

  EC   = strtok(sensorstring_array, ",");
  TDS  = strtok(NULL, ",");
  SAL  = strtok(NULL, ",");
  GRAV = strtok(NULL, ",");

  Serial.print("EC:");
  Serial.println(EC);

  Serial.print("TDS:");
  Serial.println(TDS);

  Serial.print("SAL:");
  Serial.println(SAL);

  Serial.print("GRAV:");
  Serial.println(GRAV);
  Serial.println();

  // Mostrar en LCD (solo EC para que quepa correctamente)
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("EC:");
  lcd.setCursor(0,1);
  lcd.print(EC);
}
