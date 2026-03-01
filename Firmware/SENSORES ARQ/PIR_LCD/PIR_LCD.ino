#include <Wire.h>
#include <LiquidCrystal_I2C.h>

const int LEDPin = 13;   
const int PIRPin = 2;    

int val = 0;             

LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup() 
{
  pinMode(LEDPin, OUTPUT); 
  pinMode(PIRPin, INPUT);
  Serial.begin(9600);

  lcd.init();
  lcd.backlight();
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("Sensor PIR");
}

void loop()
{
  val = digitalRead(PIRPin);

  if (val == HIGH)   
  { 
    digitalWrite(LEDPin, HIGH);
    Serial.println("Sensor activado");

    lcd.setCursor(0, 1);
    lcd.print("Movimiento ON ");
  } 
  else   
  {
    digitalWrite(LEDPin, LOW);
    Serial.println("Sensor parado");

    lcd.setCursor(0, 1);
    lcd.print("Movimiento OFF");
  }

  delay(200);  // pequeño debounce visual
}
