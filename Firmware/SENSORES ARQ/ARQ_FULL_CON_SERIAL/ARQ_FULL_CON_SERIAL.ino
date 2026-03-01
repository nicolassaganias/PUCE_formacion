/*
  DATALOGGER MULTISENSOR (Arduino UNO) - sin delay()

  Pantallas LCD:
  1) Fecha y hora
  2) PIR: Gente / Vacio
  3) Sonido: BAJO / MEDIO / ALTO
  4) Temp y Hum
  5) Lux
  6) MQ2 (DIGITAL): Alarma humo SI/NO  (usa DO del modulo MQ2)
  7) MQ7: CO ppm

  SD: datos.csv con timestamp del RTC

  Calibración opcional:
  - SOLO MQ7 (macro CALIBRAR_MQ7_EN_SETUP)
*/

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_AHTX0.h>
#include <BH1750.h>
#include <RTClib.h>
#include <SPI.h>
#include <SD.h>
#include "MQ7.h"

// ===================== OPCION CALIBRAR MQ7 =====================
#define CALIBRAR_MQ7_EN_SETUP 1
// ===============================================================

// MQ2 digital invertido (si el LED "detectando" corresponde a DO=LOW)
#define MQ2_ACTIVE_LOW 1  // 1: LOW=alarma, 0: HIGH=alarma

// ------------------- Config hardware -------------------
#define LCD_ADDR 0x27
#define SD_CS    10
#define LOG_NAME "datos.csv"

const uint8_t PIR_PIN = 2;
const uint8_t MQ7_PIN = A0;
const uint8_t MQ2_DO  = A1;   // MQ2 DIGITAL OUTPUT
const uint8_t MIC_PIN = A2;

// ------------------- Timing -------------------
const uint32_t SENSOR_INTERVAL_MS = 1000;
const uint32_t LOG_INTERVAL_MS    = 5000;
const uint32_t PAGE_INTERVAL_MS   = 3000;
const uint32_t LCD_REFRESH_MS     = 250;
const uint32_t SERIAL_INTERVAL_MS = 2000;  // <-- INTERVALO SERIAL PRINT (ms)

// ------------------- Mic -------------------
const uint16_t MIC_WINDOW_MS = 60;
int MIC_TH_LOW  = 110;
int MIC_TH_HIGH = 180;

// ------------------- Objetos -------------------
LiquidCrystal_I2C lcd(LCD_ADDR, 16, 2);
Adafruit_AHTX0 aht;
BH1750 bh1750;
RTC_DS3231 rtc;

#define MQ7_VOLTAGE 5
MQ7 mq7(0, MQ7_VOLTAGE);

// ------------------- Estado sensores -------------------
float tC = 0, hPct = 0;
float lux = 0;
float mq7Ppm = 0;
uint8_t pir = 0;

bool mq2Alarm = false;

uint16_t micP2P = 0;
const char* micLevel = "BAJO";

// Mic
uint32_t micStart = 0;
int micMin = 1023, micMax = 0;

// MQ2 filtro digital
uint8_t mq2Confirm = 0;

// ------------------- Scheduler -------------------
uint32_t tSensor = 0, tLog = 0, tPage = 0, tLcd = 0, tSerial = 0;  // <-- Agregado tSerial
uint8_t page = 0;
const uint8_t PAGE_COUNT = 7;

// ------------------- LCD diff -------------------
char last0[17] = "                ";
char last1[17] = "                ";

void lcdWriteDiff(uint8_t row, const char* newLine, char* oldLine) {
  uint8_t i = 0;
  while (i < 16) {
    if (newLine[i] != oldLine[i]) {
      uint8_t start = i;
      while (i < 16 && newLine[i] != oldLine[i]) {
        oldLine[i] = newLine[i];
        i++;
      }
      lcd.setCursor(start, row);
      for (uint8_t j = start; j < i; j++) lcd.print(oldLine[j]);
    } else i++;
  }
}

static inline void fillSpaces(char* s16){ for(uint8_t i=0;i<16;i++) s16[i]=' '; s16[16]='\0'; }
static inline void twoDigits(char* o,uint8_t v){ o[0]='0'+v/10; o[1]='0'+v%10; }

const char* micLevelFromP2P(uint16_t p2p){
  if(p2p<MIC_TH_LOW) return "BAJO";
  if(p2p<MIC_TH_HIGH) return "MEDIO";
  return "ALTO";
}

void micUpdate(){
  int v=analogRead(MIC_PIN);
  if(v<micMin) micMin=v;
  if(v>micMax) micMax=v;

  uint32_t now=millis();
  if(now-micStart>=MIC_WINDOW_MS){
    micP2P=(uint16_t)(micMax-micMin);
    micLevel=micLevelFromP2P(micP2P);
    micStart=now;
    micMin=1023;
    micMax=0;
  }
}

// MQ2 DO con filtro (confirmación) + LOGICA INVERTIBLE
void mq2UpdateDigital(){
  bool raw;
#if MQ2_ACTIVE_LOW
  raw = (digitalRead(MQ2_DO) == LOW);   // LOW = alarma
#else
  raw = (digitalRead(MQ2_DO) == HIGH);  // HIGH = alarma
#endif

  if(raw){ if(mq2Confirm<5) mq2Confirm++; }
  else   { if(mq2Confirm>0) mq2Confirm--; }

  mq2Alarm = (mq2Confirm >= 3);
}

void readSensorsFast(){
  sensors_event_t h,t;
  aht.getEvent(&h,&t);
  tC=t.temperature;
  hPct=h.relative_humidity;

  lux=bh1750.readLightLevel();
  if(lux<0) lux=0;

  pir=digitalRead(PIR_PIN);

  mq7Ppm=mq7.readPpm();

  mq2UpdateDigital();
}

// ------------------- SERIAL PRINT -------------------
void serialPrintData(const DateTime& now){
  // Formato compacto tipo CSV para el monitor serial
  Serial.print(F("["));
  if(now.day()<10) Serial.print('0'); Serial.print(now.day()); Serial.print('/');
  if(now.month()<10) Serial.print('0'); Serial.print(now.month()); Serial.print('/');
  Serial.print(now.year()); Serial.print(' ');
  if(now.hour()<10) Serial.print('0'); Serial.print(now.hour()); Serial.print(':');
  if(now.minute()<10) Serial.print('0'); Serial.print(now.minute()); Serial.print(':');
  if(now.second()<10) Serial.print('0'); Serial.print(now.second());
  Serial.print(F("] "));
  
  Serial.print(F("PIR:")); Serial.print(pir);
  Serial.print(F(" | SONIDO:")); Serial.print(micLevel);
  Serial.print(F("(")); Serial.print(micP2P); Serial.print(F(")"));
  Serial.print(F(" | T:")); Serial.print(tC,1); Serial.print(F("C"));
  Serial.print(F(" | H:")); Serial.print(hPct,1); Serial.print(F("%"));
  Serial.print(F(" | LUX:")); Serial.print(lux,1);
  Serial.print(F(" | MQ2:")); Serial.print(mq2Alarm ? F("ALARMA!") : F("OK"));
  Serial.print(F(" | MQ7:")); Serial.print(mq7Ppm,1); Serial.println(F("ppm"));
}

// ------------------- SD -------------------
void ensureHeader(){
  if(!SD.exists(LOG_NAME)){
    File f=SD.open(LOG_NAME,FILE_WRITE);
    if(f){
      f.println(F("fecha,hora,pir,sonido,temp_c,hum_pct,lux,mq2_alarma,mq7_co_ppm,mic_p2p"));
      f.close();
    }
  }
}

void logRow(const DateTime& now){
  File f=SD.open(LOG_NAME,FILE_WRITE);
  if(!f) return;

  if(now.day()<10)f.print('0');f.print(now.day());f.print('/');
  if(now.month()<10)f.print('0');f.print(now.month());f.print('/');
  uint8_t yy=now.year()%100;
  if(yy<10)f.print('0');f.print(yy);
  f.print(',');

  if(now.hour()<10)f.print('0');f.print(now.hour());f.print(':');
  if(now.minute()<10)f.print('0');f.print(now.minute());f.print(':');
  if(now.second()<10)f.print('0');f.print(now.second());
  f.print(',');

  f.print(pir); f.print(',');
  f.print(micLevel); f.print(',');
  f.print(tC,1); f.print(',');
  f.print(hPct,1); f.print(',');
  f.print(lux,1); f.print(',');
  f.print(mq2Alarm?1:0); f.print(',');
  f.print(mq7Ppm,1); f.print(',');
  f.println(micP2P);

  f.close();
}

// ------------------- LCD -------------------
void renderLcd(const DateTime& now){
  char l0[17],l1[17];
  fillSpaces(l0); fillSpaces(l1);

  if(page==0){
    char dd[2],mm[2],yy[2],hh[2],mi[2],ss[2];
    twoDigits(dd,now.day());
    twoDigits(mm,now.month());
    twoDigits(yy,(uint8_t)(now.year()%100));
    twoDigits(hh,now.hour());
    twoDigits(mi,now.minute());
    twoDigits(ss,now.second());

    l0[0]=dd[0]; l0[1]=dd[1]; l0[2]='/'; l0[3]=mm[0]; l0[4]=mm[1]; l0[5]='/'; l0[6]=yy[0]; l0[7]=yy[1];
    l1[0]=hh[0]; l1[1]=hh[1]; l1[2]=':'; l1[3]=mi[0]; l1[4]=mi[1]; l1[5]=':'; l1[6]=ss[0]; l1[7]=ss[1];
  }
  else if(page==1){
    const char* t0="Sensor PIR";
    for(uint8_t i=0;t0[i]&&i<16;i++) l0[i]=t0[i];
    const char* s=pir?"Gente":"Vacio";
    for(uint8_t i=0;s[i]&&i<16;i++) l1[i]=s[i];
  }
  else if(page==2){
    const char* t0="Nivel Sonido:";
    for(uint8_t i=0;t0[i]&&i<16;i++) l0[i]=t0[i];
    for(uint8_t i=0;micLevel[i]&&i<16;i++) l1[i]=micLevel[i];
  }
  else if(page==3){
    char tmp[10];
    const char* a0="Temp: ";
    for(uint8_t i=0;a0[i];i++) l0[i]=a0[i];
    dtostrf(tC,4,1,tmp);
    for(uint8_t i=0;tmp[i]&&(6+i)<16;i++) l0[6+i]=tmp[i];
    l0[11]=' '; l0[12]='C';

    const char* a1="Hum:  ";
    for(uint8_t i=0;a1[i];i++) l1[i]=a1[i];
    dtostrf(hPct,4,1,tmp);
    for(uint8_t i=0;tmp[i]&&(6+i)<16;i++) l1[6+i]=tmp[i];
    l1[11]=' '; l1[12]='%';
  }
  else if(page==4){
    const char* t0="Luz:";
    for(uint8_t i=0;t0[i];i++) l0[i]=t0[i];
    char tmp[10];
    dtostrf(lux,6,1,tmp);
    for(uint8_t i=0;tmp[i]&&(5+i)<16;i++) l0[5+i]=tmp[i];
    l0[13]=' '; l0[14]='l'; l0[15]='x';
  }
  else if(page==5){
    const char* t0="MQ2 Humo:";
    for(uint8_t i=0;t0[i]&&i<16;i++) l0[i]=t0[i];
    const char* s=mq2Alarm?"ALARMA: SI":"ALARMA: NO";
    for(uint8_t i=0;s[i]&&i<16;i++) l1[i]=s[i];
  }
  else{
    const char* t0="CO (MQ7):";
    for(uint8_t i=0;t0[i]&&i<16;i++) l0[i]=t0[i];
    const char* t1="PPM: ";
    for(uint8_t i=0;t1[i]&&i<16;i++) l1[i]=t1[i];
    char tmp[10];
    dtostrf(mq7Ppm,6,1,tmp);
    for(uint8_t i=0;tmp[i]&&(5+i)<16;i++) l1[5+i]=tmp[i];
  }

  lcdWriteDiff(0,l0,last0);
  lcdWriteDiff(1,l1,last1);
}

void haltMsg(const __FlashStringHelper* m){
  lcd.setCursor(0,1);
  lcd.print(m);
  while(1){}
}

void setup(){
  pinMode(PIR_PIN,INPUT);
  pinMode(MQ2_DO,INPUT);

  Serial.begin(9600);
  while(!Serial && millis()<3000); // Espera Serial hasta 3s (para USB nativo)

  Wire.begin();
  lcd.init();
  lcd.backlight();

  lcd.setCursor(0,0); lcd.print(F("Iniciando..."));
  lcd.setCursor(0,1); lcd.print(F("Datalogger UNO"));

  if(!rtc.begin()) haltMsg(F("RTC ERROR"));
  if(rtc.lostPower()) rtc.adjust(DateTime(F(__DATE__),F(__TIME__)));

  if(!SD.begin(SD_CS)) haltMsg(F("SD ERROR CS10"));
  ensureHeader();

  if(!aht.begin()) haltMsg(F("AHT ERROR"));
  if(!bh1750.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) haltMsg(F("BH1750 ERROR"));

#if CALIBRAR_MQ7_EN_SETUP
  lcd.clear();
  lcd.setCursor(0,0); lcd.print(F("Calibrando MQ7"));
  lcd.setCursor(0,1); lcd.print(F("Aire limpio..."));
  mq7.calibrate();
  lcd.clear();
  lcd.setCursor(0,0); lcd.print(F("MQ7 OK"));
  delay(800);
#endif

  micStart=millis();
  micMin=1023;
  micMax=0;

  uint32_t ms=millis();
  tSensor=ms;
  tLog=ms;
  tPage=ms;
  tLcd=ms;
  tSerial=ms;  // <-- Inicializar scheduler Serial

  readSensorsFast();

  for(uint8_t i=0;i<16;i++){ last0[i]='?'; last1[i]='?'; }
  last0[16]='\0';
  last1[16]='\0';

  lcd.clear();
}

void loop(){
  uint32_t ms=millis();

  micUpdate();

  if(ms-tSensor>=SENSOR_INTERVAL_MS){
    tSensor+=SENSOR_INTERVAL_MS;
    readSensorsFast();
  }

  if(ms-tPage>=PAGE_INTERVAL_MS){
    tPage+=PAGE_INTERVAL_MS;
    page=(page+1)%PAGE_COUNT;
  }

  if(ms-tLcd>=LCD_REFRESH_MS){
    tLcd+=LCD_REFRESH_MS;
    renderLcd(rtc.now());
  }

  // <-- NUEVO: Serial Print periódico
  if(ms-tSerial>=SERIAL_INTERVAL_MS){
    tSerial+=SERIAL_INTERVAL_MS;
    serialPrintData(rtc.now());
  }

  if(ms-tLog>=LOG_INTERVAL_MS){
    tLog+=LOG_INTERVAL_MS;
    logRow(rtc.now());
  }
}