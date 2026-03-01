#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <OneWire.h>

#define TdsSensorPin A1
#define ONE_WIRE_BUS 2

#define VREF 5.0
#define SCOUNT 30

LiquidCrystal_I2C lcd(0x27, 16, 2);
OneWire ds(ONE_WIRE_BUS);

// ---------- Variables ----------
int analogBuffer[SCOUNT];
int analogBufferTemp[SCOUNT];
int analogBufferIndex = 0, copyIndex = 0;

float averageVoltage = 0;
float tdsValue = 0;
float temperature = 25.0;

// ---------- Setup ----------
void setup()
{
  Serial.begin(115200);
  pinMode(TdsSensorPin, INPUT);

  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("TDS Meter");
  lcd.setCursor(0, 1);
  lcd.print("Init...");
  delay(1500);
  lcd.clear();
}

// ---------- Loop ----------
void loop()
{
  static unsigned long analogSampleTimepoint = millis();
  if (millis() - analogSampleTimepoint > 40)
  {
    analogSampleTimepoint = millis();
    analogBuffer[analogBufferIndex] = analogRead(TdsSensorPin);
    analogBufferIndex++;
    if (analogBufferIndex == SCOUNT)
      analogBufferIndex = 0;
  }

  static unsigned long printTimepoint = millis();
  if (millis() - printTimepoint > 800)
  {
    printTimepoint = millis();

    temperature = readDS18B20();

    for (copyIndex = 0; copyIndex < SCOUNT; copyIndex++)
      analogBufferTemp[copyIndex] = analogBuffer[copyIndex];

    averageVoltage = getMedianNum(analogBufferTemp, SCOUNT) * VREF / 1024.0;

    float compensationCoefficient = 1.0 + 0.02 * (temperature - 25.0);
    float compensationVoltage = averageVoltage / compensationCoefficient;

    tdsValue = (133.42 * compensationVoltage * compensationVoltage * compensationVoltage
              - 255.86 * compensationVoltage * compensationVoltage
              + 857.39 * compensationVoltage) * 0.5;

    // Serial
    Serial.print("Temp: ");
    Serial.print(temperature, 1);
    Serial.print(" C | TDS: ");
    Serial.print(tdsValue, 0);
    Serial.println(" ppm");

    // LCD
    lcd.setCursor(0, 0);
    lcd.print("TDS:");
    lcd.print(tdsValue, 0);
    lcd.print(" ppm   ");

    lcd.setCursor(0, 1);
    lcd.print("Temp:");
    lcd.print(temperature, 1);
    lcd.print((char)223);
    lcd.print("C   ");
  }
}

// ---------- DS18B20 (OneWire puro) ----------
float readDS18B20()
{
  byte data[9];
  byte addr[8];

  if (!ds.search(addr))
  {
    ds.reset_search();
    return temperature; // mantiene último valor válido
  }

  if (OneWire::crc8(addr, 7) != addr[7])
    return temperature;

  ds.reset();
  ds.select(addr);
  ds.write(0x44); // start conversion
  delay(750);

  ds.reset();
  ds.select(addr);
  ds.write(0xBE); // read scratchpad

  for (byte i = 0; i < 9; i++)
    data[i] = ds.read();

  int16_t raw = (data[1] << 8) | data[0];
  return (float)raw / 16.0;
}

// ---------- Mediana ----------
int getMedianNum(int bArray[], int iFilterLen)
{
  int bTab[iFilterLen];
  for (byte i = 0; i < iFilterLen; i++)
    bTab[i] = bArray[i];

  for (int j = 0; j < iFilterLen - 1; j++)
  {
    for (int i = 0; i < iFilterLen - j - 1; i++)
    {
      if (bTab[i] > bTab[i + 1])
      {
        int tmp = bTab[i];
        bTab[i] = bTab[i + 1];
        bTab[i + 1] = tmp;
      }
    }
  }

  if (iFilterLen & 1)
    return bTab[(iFilterLen - 1) / 2];
  else
    return (bTab[iFilterLen / 2] + bTab[iFilterLen / 2 - 1]) / 2;
}
