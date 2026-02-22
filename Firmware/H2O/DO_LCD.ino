// #define USE_PULSE_OUT

#ifdef USE_PULSE_OUT
  #include "do_iso_surveyor.h"       
  Surveyor_DO_Isolated DO = Surveyor_DO_Isolated(A0);         
#else
  #include "do_surveyor.h"
  Surveyor_DO DO = Surveyor_DO(A0);
#endif

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

uint8_t user_bytes_received = 0;
const uint8_t bufferlen = 32;
char user_data[bufferlen];

void parse_cmd(char* string) {
  strupr(string);
  String cmd = String(string);
  if(cmd.startsWith("CAL")){
    int index = cmd.indexOf(',');
    if(index != -1){
      String param = cmd.substring(index+1, cmd.length());
      if(param.equals("CLEAR")){
        DO.cal_clear();
        Serial.println("CALIBRATION CLEARED");
      }
    }
    else{
      DO.cal();
      Serial.println("DO CALIBRATED");
    }
  }
}

void setup() {
  Serial.begin(9600);
  delay(200);

  DO.begin();

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("DO Monitor");
  delay(1500);
  lcd.clear();
}

void loop() {

  if (Serial.available() > 0) {
    user_bytes_received = Serial.readBytesUntil(13, user_data, sizeof(user_data));
  }

  if (user_bytes_received) {
    parse_cmd(user_data);
    user_bytes_received = 0;
    memset(user_data, 0, sizeof(user_data));
  }

  float do_value = DO.read_do_percentage();

  Serial.print("DO: ");
  Serial.print(do_value);
  Serial.println(" %");

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("DO (%):");
  lcd.setCursor(0,1);
  lcd.print(do_value);
  lcd.print(" %");

  delay(1000);
}
