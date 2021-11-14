#include "EEPROMex.h"
#include "LiquidCrystal_I2C.h"

#define buttonPower 2
#define buttonCalibration 3
#define pinLCDpower 12
#define pinLCD 11
#define pinPhotodiode 17
#define pinPhotodiodeSensor A0

LiquidCrystal_I2C lcd(0x27, 16, 2);

#define coinTypeAmount 4
float coinTypeValue[coinTypeAmount] = {0.05, 0.1, 0.25, 0.5};
int coinAmount[coinTypeAmount];
float coinTotalMoney = 0;
String currency = "UAH";

int signalCoinType[coinTypeAmount];
byte signalDefault;
int signalSensorData, signalSensorDataPrevious;

unsigned long timerButtonPowerActive, timerReset;
int timerSleepMode = 10000;

boolean caseRecognition, caseSleepMode = true;
boolean caseCoinInserted = false;

void setup() {
  Serial.begin(9600);
  pinMode(buttonPower, INPUT_PULLUP);
  pinMode(buttonCalibration, INPUT_PULLUP);
  pinMode(pinLCDpower, OUTPUT);
  pinMode(pinLCD, OUTPUT);
  pinMode(pinPhotodiode, OUTPUT);

  lcd.init();
  lcd.backlight();

  digitalWrite(pinLCDpower, 1);
  digitalWrite(pinLCD, 1);
  digitalWrite(pinPhotodiode, 1);

  attachInterrupt(0, switchSleepModeOff, CHANGE);

  signalDefault = analogRead(pinPhotodiodeSensor);

  if (!digitalRead(buttonCalibration)) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Service");
    delay(100);
    timerReset = millis();
    while (1) {
      if (millis() - timerReset > 3000) {
        for (byte i = 0; i < coinTypeAmount; i++) {
          coinAmount[i] = 0;
          EEPROM.writeInt(20 + i * 2, 0);
        }
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Reset complete");
        delay(100);
      }
      if (digitalRead(buttonCalibration)) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Calibration");
        break;
      }
    }
    while (1) {
      for (byte i = 0; i < coinTypeAmount; i++) {
        lcd.setCursor(0, 1);
        lcd.print(coinTypeValue[i]);
        lcd.print(" ");
        lcd.print(currency);
        signalSensorDataPrevious = signalDefault;
        while (1) {
          signalSensorData = analogRead(pinPhotodiodeSensor);
          if (signalSensorData > signalSensorDataPrevious) signalSensorDataPrevious = signalSensorData;
          if (signalSensorData - signalDefault > 3) caseCoinInserted = true;
          if (caseCoinInserted && (abs(signalSensorData - signalDefault)) < 2) {
            signalCoinType[i] = signalSensorDataPrevious;
            EEPROM.writeInt(i * 2, signalCoinType[i]);
            caseCoinInserted = false;
            break;
          }
        }
      }
      break;
    }
  }

  for (byte i = 0; i < coinTypeAmount; i++) {
    signalCoinType[i] = EEPROM.readInt(i * 2);
    coinAmount[i] = EEPROM.readInt(20 + i * 2);
    coinTotalMoney += coinAmount[i] * coinTypeValue[i];
  }
  timerButtonPowerActive = millis();
}

void switchSleepModeOn() {
  for (byte i = 0; i < coinTypeAmount; i++) {
    EEPROM.updateInt(20 + i * 2, coinAmount[i]);
  }
  caseSleepMode = true;
  digitalWrite(pinLCDpower, 0);
  digitalWrite(pinLCD, 0);
  digitalWrite(pinPhotodiode, 0);
  delay(100);
}

void switchSleepModeOff() {
  digitalWrite(pinLCDpower, 1);
  digitalWrite(pinLCD, 1);
  digitalWrite(pinPhotodiode, 1);
  timerButtonPowerActive = millis();
}

void loop() {
  if (caseSleepMode) {
    delay(500);
    lcd.init();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Total money");
    lcd.setCursor(0, 1);
    lcd.print(coinTotalMoney);
    lcd.print(" ");
    lcd.print(currency);
    signalDefault = analogRead(pinPhotodiodeSensor);
    caseSleepMode = false;
  }
  signalSensorDataPrevious = signalDefault;
  while (1) {
    signalSensorData = analogRead(pinPhotodiodeSensor);
    if (signalSensorData > signalSensorDataPrevious) signalSensorDataPrevious = signalSensorData;
    if (signalSensorData - signalDefault > 3) caseCoinInserted = true;
    if (caseCoinInserted && (abs(signalSensorData - signalDefault)) < 2) {
      caseRecognition = false;
      for (byte i = 0; i < coinTypeAmount; i++) {
        int delta = abs(signalSensorDataPrevious - signalCoinType[i]);
        if (delta < 30) {
          coinTotalMoney += coinTypeValue[i];
          lcd.setCursor(0, 1); lcd.print(coinTotalMoney);
          coinAmount[i]++;
          caseRecognition = true;
          break;
        }
      }
      caseCoinInserted = false;
      timerButtonPowerActive = millis();
      break;
    }

    if (millis() - timerButtonPowerActive > timerSleepMode) {
      switchSleepModeOn();
      break;
    }

    while (!digitalRead(buttonPower)) {
      if (millis() - timerButtonPowerActive > 2000) {
        lcd.clear();

        for (byte i = 0; i < coinTypeAmount; i) {
          lcd.setCursor(i * 3, 0);
          lcd.print((int)coinTypeValue[i]);
          lcd.setCursor(i * 3, 1);
          lcd.print(coinAmount[i]);
        }
      }
    }
  }
}
