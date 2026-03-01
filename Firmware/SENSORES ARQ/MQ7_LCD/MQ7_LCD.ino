/*	
	MQ7_Example con LCD 16x2 I2C (0x27)
*/

#include "MQ7.h"
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define A_PIN 0
#define VOLTAGE 5

// Init MQ7
MQ7 mq7(A_PIN, VOLTAGE);

// Init LCD (direccion 0x27, 16 columnas, 2 filas)
LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup() {
	Serial.begin(9600);
	while (!Serial) {
		;
	}

	// Inicializar LCD
	lcd.init();
	lcd.backlight();
	lcd.clear();

	lcd.setCursor(0, 0);
	lcd.print("Calibrando MQ7");

	Serial.println("");
	Serial.println("Calibrating MQ7");

	mq7.calibrate();   // calcula R0

	Serial.println("Calibration done!");

	lcd.clear();
	lcd.setCursor(0, 0);
	lcd.print("Calibracion OK");
	delay(2000);
	lcd.clear();
}
 
void loop() {

	float ppm = mq7.readPpm();

	// Serial
	Serial.print("PPM = ");
	Serial.println(ppm);

	// LCD
	lcd.setCursor(0, 0);
	lcd.print("CO (MQ7):     ");

	lcd.setCursor(0, 1);
	lcd.print("PPM: ");
	lcd.print(ppm, 1);   // 1 decimal
	lcd.print("     ");     // limpia restos

	delay(1000);
}
