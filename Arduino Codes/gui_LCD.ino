#include <MFRC522.h>
#include <URTouch.h>
#include <SPI.h>
#include <ILI9341_due_config.h>
#include <ILI9341_due.h>
#include "fonts\Arial_bold_14.h"
#include <Wire.h>
#include <DS1302.h>
#include "batt.h"

#define kCePin 5    // Chip Enable
#define kIoPin 6    // Input/Output
#define kSclkPin 7  // Serial Clock
DS1302 rtc(kCePin, kIoPin, kSclkPin);

//define the lcd and touch pins
#define TFT_RST 8
#define TFT_DC 9
#define TFT_CS 10
ILI9341_due tft = ILI9341_due(TFT_CS, TFT_DC, TFT_RST);
//URTouch(byte tclk, byte tcs, byte tdin, byte dout, byte irq);
URTouch  myTouch(30, 28, 26, 24, 22);

#define RST_PIN 3 //define rfid pins
#define SS_PIN 2
MFRC522 mfrc522(SS_PIN, RST_PIN);

#define BUZZER 11 //define BUZZER pin
#define EEPR 0x50 //Address of 24LC64 eeprom chip i2c

uint16_t lcd_p = 0, color_t, color_b; //variable for lcd state and for color text and background color
byte readCard[4] ; //array for read UID (4 bytes) from rfid
uint16_t NumbSen = 0; //to save values received from the battery
uint16_t tempMax, tempMin, smokeM, gasM; //The limits of the alarm values
uint16_t alm_stt; // sign for the alarm mode
byte chk = 0; // sign when cheak sensor
byte tOff = 0; // sign when need to turn off alarm
uint16_t color_p, ncolor_p;
String qqq = "", aaa = "";//support strings
int i, j, err;//for loops
char c = 0;
String ser , topic, temp , checkA, smoke , gas, door, motion1, motion2, tempBatt , smokeBatt , gasBatt , doorBatt , motion1Batt , motion2Batt ; //for Uart
int ut = 0, ug = 0, us = 0, ud = 0, um1 = 0, um2 = 0;

String dayAsString(const Time::Day day) {
  switch (day) {
    case Time::kSunday: return "Sunday";
    case Time::kMonday: return "Monday";
    case Time::kTuesday: return "Tuesday";
    case Time::kWednesday: return "Wednesday";
    case Time::kThursday: return "Thursday";
    case Time::kFriday: return "Friday";
    case Time::kSaturday: return "Saturday";
  }
  return "(unknown day)";
}

void drawLcd1() {
  lcd_p = 1;
  tft.fillScreen(ILI9341_BLACK);

  Time t = rtc.time(); // Get the current time and date from the chip.
  const String day = dayAsString(t.day);   // Name the day of the week.

  tft.setTextScale(8);
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  if (t.min < 10) qqq = String(String(t.hr, DEC) + ":0" + String(t.min, DEC));
  else qqq = String(String(t.hr, DEC) + ":" + String(t.min, DEC));
  if (t.hr < 10) qqq = String("0" + qqq);
  tft.printAligned(qqq, gTextAlignTopCenter);

  if (alm_stt == 0) {
    qqq = ""; aaa = "Stay";
  }
  else if (alm_stt == 1) {
    qqq = ""; aaa = "Night";
  }
  else if (alm_stt == 2) {
    qqq = ""; aaa = "Away";
  }
  else if (alm_stt == 3) {
    qqq = "Stay"; aaa = "Baby";
  }
  else if (alm_stt == 4) {
    qqq = "Night"; aaa = "Baby";
  }

  tft.setTextScale(1);
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  tft.printAt(qqq, 0 , 240 - 14 * 2);
  tft.printAt(aaa, 0, 240 - 14);

  tft.setTextScale(3);
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  qqq = String(String(t.date, DEC) + "-" + String(t.mon, DEC) + "-" + String(t.yr, DEC));
  tft.printAtPivoted(qqq, 320 / 2, 14 * 8 , gTextPivotTopCenter);
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  tft.printAtPivoted(day, 160, 14 * 8  + 3 * 14, gTextPivotTopCenter);

  checkBatt();
}

void drawLcd2() {
  lcd_p = 2;
  tft.fillScreen(color_b);
  tft.setTextColor(color_t, color_b);
  tft.setTextScale(2);

  for (i = 1; i < 3; i++) {
    tft.drawRoundRect(20 * i + 130 * (i - 1), (240 - 130) / 2, 130, 130, 10, color_t); //draw 2 Squares of size 130 in 1 row
  }
  tft.printAt("Details", 20 + (130 - 7 * 14) / 2, (240 - 130) / 2 + (130 - 28) / 2); //button 1
  tft.printAt("Set", 20 * 2 + 130 + (130 - 3 * 20) / 2, 55 + (130 - 28 * 2) / 2); //button 2
  tft.printAt("Alarm", 20 * 2 + 130 + (130 - 5 * 20) / 2, 55 + (130 - 28 * 2) / 2 + 28);

  drawReturn();
}

void drawLcd3() {
  lcd_p = 3;
  tft.fillScreen(color_b);
  tft.setTextScale(1);

  qqq = String("temperature:"  + temp);
  tft.printAt(qqq, 0, 57);
  qqq = String("smoke:" + smoke);
  tft.printAt(qqq, 0, 77);
  qqq = String("gas:" +  gas);
  tft.printAt(qqq, 0, 97);
  qqq = String("door:" +  door);
  tft.printAt(qqq, 0, 117);
  qqq = String("motion1:" +  motion1);
  tft.printAt(qqq, 0, 137);
  qqq = String("motion2:" +  motion2);
  tft.printAt(qqq, 0, 157);

  drawReturn();

  tft.setTextScale(2);
  tft.drawRoundRect(20 * 2 + 130 , (240 - 130) / 2, 130, 130, 10, color_t);
  tft.printAt("Battery", 180, 55 + 37);
  tft.printAt("Percent", 180, 55 + 37 + 28);
}

void drawLcd4() {
  lcd_p = 4;
  tft.fillScreen(color_b);
  tft.printAligned("Battery Details:", gTextAlignTopCenter);

  tft.setTextScale(1);
  qqq = String("temperature:"  + tempBatt);
  qqq += "%";
  tft.printAt(qqq, 0, 57);
  qqq = String("smoke:" + smokeBatt);
  qqq += "%";
  tft.printAt(qqq, 0, 77);
  qqq = String("gas:" +  gasBatt);
  qqq += "%";
  tft.printAt(qqq, 0, 97);
  qqq = String("door:" +  doorBatt);
  qqq += "%";
  tft.printAt(qqq, 0, 117);
  qqq = String("motion1:" +  motion1Batt);
  qqq += "%";
  tft.printAt(qqq, 0, 137);
  qqq = String("motion2:" +  motion2Batt);
  qqq += "%";
  tft.printAt(qqq, 0, 157);

  drawReturn();
}

void drawLcd5() {
  lcd_p = 5;
  tft.fillScreen(color_b);
  tft.setTextScale(2);
  tft.printAt("Please Attach", 50, 30);
  tft.printAt("Your Chip:", 80, 58);

  drawReturn();
  if (err == 1) tft.printAt("Error", (320 - 5 * 10) / 2, 180);
  if (err == 2) tft.printAt("Last try", (320 - 8 * 10) / 2 , 180);
  if (err >= 3) tft.printAt("Alarm on", (320 - 8 * 10) / 2 , 180);
}

void drawLcd6() {
  lcd_p = 6;
  tft.fillScreen(color_b);
  tft.setTextScale(2);
  tft.printAligned("Please Choose Option", gTextAlignTopCenter);

  for (j = 1; j < 3; j++) {
    for (i = 1; i < 3; i++) {
      tft.drawRoundRect(70 * i + 50 * (i - 1), 40 + 100 * (j - 1), 80, 80, 10, color_t); //draw 4 Squares of size 80 in 2 row
    }
  }
  drawReturn();

  tft.printAt("Alarm", 85 , 66);//button 1
  tft.printAt("Mode", 90 , 80);
  tft.printAt("Choose", 200 , 66 );//button 2
  tft.printAt("Color", 205 , 80);
  tft.printAt("Add", 95 , 166 );//button 3
  tft.printAt("Chip", 90 , 180);
  tft.printAt("Remove", 200 , 166 );//button 4
  tft.printAt("Chip", 210 , 180);
}

void drawLcd7() {
  lcd_p = 7;
  tft.fillScreen(color_b);
  tft.setTextScale(2);
  tft.printAligned("please choose mode", gTextAlignTopCenter);
  drawReturn();

  for (j = 0; j < 2; j++) {
    for (i = 1; i < 4; i++) {
      tft.drawRoundRect(50 * j + 20 * i + 80 * (i - 1), 50 + 100 * j, 80, 80, 10, color_t); //draw 5 Squares of size 80, 3 in first row, 2 in second row
      if ((j == 1) && (i == 2))break;
    }
  }
  tft.printAt("Stay", 40 , 83 );//button 1
  tft.printAt("Night", 135 , 83);//button 2
  tft.printAt("Away", 240 , 83);//button 3
  tft.printAt("Stay", 90 , 176);//button 4
  tft.printAt("Baby", 90 , 190);
  tft.printAt("Night", 185 , 176);//button 5
  tft.printAt("Baby", 190 , 190);
}

void drawLcd8() {
  lcd_p = 8;
  tft.fillScreen(color_b);
  tft.setTextScale(2);
  tft.printAligned("please choose color", gTextAlignTopCenter);
  drawReturn();
  uint16_t colArr[12] = {ILI9341_BLUE, ILI9341_BROWN, ILI9341_PURPLE, ILI9341_ORANGE, ILI9341_YELLOW, ILI9341_GREEN, ILI9341_PINK, ILI9341_TEAL, ILI9341_VIOLET, ILI9341_CYAN, ILI9341_LIME};
  int x = 0;

  for (j = 0; j < 3; j++) {
    for (i = 0; i < 4; i++) {
      if ((j == 2) && (i == 0))i++;
      tft.fillRoundRect(24 + (i * 74) , 40 + 70 * j, 50, 50, 10, colArr[x]);
      tft.drawRoundRect(24 + (i * 74), 40 + 70 * j, 50, 50, 10, color_t);
      x++;
    }
  }
}

void drawLcd9() {
  lcd_p = 9;
  tft.fillScreen(color_b);
  tft.setTextScale(2);
  tft.printAt("Please Attach", 50, 30);
  tft.printAt("new Chip:", 80, 58);
  drawReturn();
}

void drawLcd10() {
  lcd_p = 10;
  tft.fillScreen(color_b);
  tft.setTextScale(2);
  tft.printAt("Please Attach", 50, 30);
  tft.printAt("the chip you want", 20, 58);
  tft.printAt("to delete:", 70, 86);
  drawReturn();
}

void drawLcd11() {
  lcd_p = 11;
  tft.setTextScale(1);
  tft.fillScreen(color_b);
  //draw 2 Squares of size 130 in 1 row
  for (i = 1; i < 3; i++) {
    tft.drawRoundRect(20 * i + 130 * (i - 1), (240 - 130) / 2, 130, 130, 10, color_t);
  }
  tft.printAt("configure", 20 + (130 - 8 * 10) / 2, 55 + (130 - 14 * 3) / 2); //button 1
  tft.printAt("sensor", 20 + (130 - 6 * 10) / 2, 55 + (130 - 14 * 3) / 2 + 14);
  tft.printAt("and clock", 20 + (130 - 7 * 10) / 2, 55 + (130 - 14 * 3) / 2 + 14 * 2);
  tft.printAt("cheak", 20 * 2 + 130 + (130 - 5 * 10) / 2, (240 - 130) / 2 + (130 - 14 * 2) / 2); //button 2
  tft.printAt("system", 20 * 2 + 130 + (130 - 6 * 10) / 2, (240 - 130) / 2 + (130 - 14 * 2) / 2 + 14);
  drawReturn();
}

void drawLcd12() {
  lcd_p = 12;
  tft.fillScreen(color_b);
  tft.setTextScale(2);
  tft.setTextColor(color_t, color_b);
  tft.printAligned("choose unit:", gTextAlignTopCenter);
  drawReturn();

  //draw 5 Squares of size 80, 3 in first row, 2 in second row
  for (j = 0; j < 2; j++) {
    for (i = 1; i < 4; i++) {
      tft.drawRoundRect(50 * j + 20 * i + 80 * (i - 1), 50 + 100 * j, 80, 80, 10, color_t);
      if ((j == 1) && (i == 2))break;
    }
  }
  tft.printAt("Temp", 40 , 83 );
  tft.printAt("Gas", 145 , 83);
  tft.printAt("Clock", 240 , 83);
  tft.printAt("Smoke", 85 , 183);
  tft.printAt("Date", 190 , 183);
}

void drawLcd13() {
  lcd_p = 13;
  tft.setTextScale(1);
  tft.fillScreen(color_b);
  tft.printAligned(qqq, gTextAlignTopCenter);
  //draw 10 Squares of size 52, 5 in first row, 5 in second row
  for (j = 0; j < 2; j++) {
    for (i = 0; i < 5; i++) {
      tft.fillRoundRect(10 + (i * 62), 25 + 62 * j, 52, 52, 5, color_b);
      tft.drawRoundRect(10 + (i * 62), 25 + 62 * j, 52, 52, 5, color_t);
      tft.printAt(String(i + 5 * j), 32 + (i * 62), 45 + 62 * j);
    }
  }
  for (i = 0; i < 2; i++) {
    tft.fillRoundRect(10 + (i * 186), 149, 114, 52, 5, color_b);
    tft.drawRoundRect(10 + (i * 186), 149, 114, 52, 5, color_t);
  }
  tft.printAt("Clear", 42, 168);
  tft.printAt("Enter", 238, 168);
  drawReturn();
}

void drawLcd14() {
  lcd_p = 14;
  tft.setTextScale(2);
  tft.fillScreen(color_b);
  //draw 2 Squares of size 130 in 1 row
  for (i = 1; i < 3; i++) {
    tft.fillRoundRect(20 * i + 130 * (i - 1) , (240 - 130) / 2, 130, 130, 10, color_b);
    tft.drawRoundRect(20 * i + 130 * (i - 1), (240 - 130) / 2, 130, 130, 10, color_t);
  }
  tft.printAt("Check", 20 + (130 - 5 * 20) / 2, (240 - 130) / 2 + (130 - 28 * 2) / 2);
  tft.printAt("Units", 20 + (130 - 5 * 20) / 2, (240 - 130) / 2 + (130 - 28 * 2) / 2 + 28);
  tft.printAt("Check", 20 * 2 + 130 + (130 - 5 * 20) / 2, 55 + (130 - 28 * 2) / 2);
  tft.printAt("Buzzer", 20 * 2 + 130 + (130 - 6 * 20) / 2, 55 + (130 - 28 * 2) / 2 + 28);
  drawReturn();
}

void drawLcd15() {
  lcd_p = 15;
  tft.setTextScale(2);
  tft.fillScreen(color_b);
  tft.printAligned("Unit check:", gTextAlignTopCenter);
  drawReturn();
  if (ut == 1) tft.printAt("temp:work", 0, 57);
  else if (ut == 2)tft.printAt("temp:not work", 0, 57);
  if (us == 1)tft.printAt("smoke:work", 0, 77);
  else if (us == 2)tft.printAt("smoke:not work", 0, 77);
  if (ug == 1) tft.printAt("gas:work", 0, 97);
  else   if (ug == 2) tft.printAt("gas:not work", 0, 97);
  if (ud == 1)tft.printAt("door:work", 0, 117);
  else   if (ud == 2)tft.printAt("door:not work", 0, 117);
  if (um1 == 1)tft.printAt("motion1:work", 0, 137);
  else   if (um1 == 2)tft.printAt("motion1:not work", 0, 137);
  if (um2 == 1)tft.printAt("motion2:work", 0, 157);
  else   if (um2 == 2)tft.printAt("motion2:not work", 0, 157);
}

void drawReturn() {
  tft.fillRoundRect(0 , 210, 30, 30, 10, color_b);
  tft.drawRoundRect(0 , 210, 30, 30, 10, color_t);
  tft.setTextScale(1);
  tft.printAt("<-", 6 , 220);
}

void colors() {
  if (color_p == 1)color_b = ILI9341_BLUE;
  else if (color_p == 2)color_b = ILI9341_BROWN;
  else if (color_p == 3)color_b = ILI9341_PURPLE;
  else if (color_p == 4)color_b = ILI9341_ORANGE;
  else if (color_p == 5)color_b = ILI9341_YELLOW;
  else if (color_p == 6)color_b = ILI9341_GREEN;
  else if (color_p == 7)color_b = ILI9341_PINK;
  else if (color_p == 8)color_b = ILI9341_TEAL;
  else if (color_p == 9)color_b = ILI9341_VIOLET;
  else if (color_p == 10)color_b = ILI9341_CYAN;
  else if (color_p == 11)color_b = ILI9341_LIME;
  if ((color_p == 1) || (color_p == 2) || (color_p == 3) || (color_p == 6) || (color_p == 8))color_t = ILI9341_WHITE;
  else if ((color_p == 4) || (color_p == 5) || (color_p == 7) || (color_p == 9) || (color_p == 10) || (color_p == 11))color_t = ILI9341_BLACK;

  tft.setTextColor(color_t, color_b);
}

int readUIDchip() {
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    for ( i = 0; i < 4; i++)
      readCard[i] = mfrc522.uid.uidByte[i];
    return 1;
  }
  else return 0;
}

void add_chip() {
  int p = 0;
  if (readUIDchip()) {
    p = cheak_chip();
    if (p == 0) {
      p = readEEPROM(EEPR, 9);
      for (i = 0; i < 4; i++) {
        writeEEPROM(EEPR, 14 + p * 4 + i, readCard[i]);
      }
      writeEEPROM(EEPR, 9, p + 1);
      tft.printAt("chip added", 125 , 176);
      delay(3000);
      drawLcd6();
    }
    else {
      tft.printAt("the chip already exists", 70 , 176);
      delay(3000);
      drawLcd9();
    }
  }
}

void del_chip() {
  int p = 0;
  p = readEEPROM(EEPR, 9); // Reads from Address 9 the number of chips
  if (p == 1) {
    tft.printAt("could not delete chip", 80 , 160);
    tft.printAt("only one chip definded in the system", 20 , 174);
  }
  else {
    if (readUIDchip()) {
      i = cheak_chip();
      if (i == 0) {
        tft.printAt("the chip does not exist", 0 , 176);
        tft.printAt("in the system", 30 , 190);
        delay(3000);
        drawLcd10();
      }
      else {
        for (; i < p * 4 + 14 - 4; i++) {
          j = readEEPROM(EEPR, i + 4);
          writeEEPROM(EEPR, i, j);
        }
        writeEEPROM(EEPR, 9, p - 1);
        tft.printAt("chip deleted", 118 , 176);
        delay(3000);
        drawLcd6();
      }
    }
  }
}

byte cheak_chip() {
  int chip_cheak[4];
  int amount_chip;
  amount_chip = readEEPROM(EEPR, 9);
  for (i = 0; i < amount_chip; i++) {
    for (j = 0; j < 4; j++) {
      chip_cheak[j] = readEEPROM(EEPR, 14 + j + i * 4);
    }
    if ((chip_cheak[0] == readCard[0]) && (chip_cheak[1] == readCard[1]) && (chip_cheak[2] == readCard[2]) && (chip_cheak[3] == readCard[3]))
    {
      return 14 + i * 4;
    }
  }
  return 0;
}

void writeEEPROM(int deviceaddress, unsigned int eeaddress, byte data )
{
  Wire.beginTransmission(deviceaddress);
  Wire.write((int)(eeaddress >> 8));   // MSB
  Wire.write((int)(eeaddress & 0xFF)); // LSB
  Wire.write(data);
  Wire.endTransmission();
  delay(5);
}

byte readEEPROM(int deviceaddress, unsigned int eeaddress )
{
  byte rdata = 0xFF;

  Wire.beginTransmission(deviceaddress);
  Wire.write((int)(eeaddress >> 8));   // MSB
  Wire.write((int)(eeaddress & 0xFF)); // LSB
  Wire.endTransmission();
  Wire.requestFrom(deviceaddress, 1);
  if (Wire.available()) rdata = Wire.read();
  return rdata;
}

void waitForIt(uint16_t bx1, uint16_t by1, uint16_t bw, uint16_t bh, uint16_t bd)
{
  tft.drawRoundRect(bx1, by1, bw , bh, bd, ILI9341_RED);
  while (myTouch.dataAvailable())
    myTouch.read();
  tft.drawRoundRect(bx1, by1, bw , bh, bd, color_t);
}

void runTime(void) {
  tft.setTextScale(3);
  for (int i = 30; i > 0; i--) {
    String tPrint = String(i, DEC);
    tft.printAt(tPrint, 130 , 198);
    delay(1000);
    tft.fillRoundRect(130, 198, 60, 42, 0, ILI9341_BLACK);
  }
}

void checkBatt(void) {
  if (lcd_p == 1) {
    int tbd = tempBatt.toInt();
    int sbd = smokeBatt.toInt();
    int dbd = doorBatt.toInt();
    int m1bd = motion1Batt.toInt();
    int m2bd = motion2Batt.toInt();
    int gbd = gasBatt.toInt();
    if ((tbd <= 20) || (sbd <= 20) || (dbd <= 20) || (m1bd <= 20) || (m2bd <= 20) || (gbd <= 20))
      tft.drawImage(batt, 300, 208, battWidth, battHeight);
    else
      tft.fillRoundRect(300 , 208, battWidth, battHeight, 0, ILI9341_BLACK);
  }
}

void checkSensor(void) {
  int gasDec = gas.toInt();
  int smokeDec = smoke.toInt();
  int tempDec = temp.toInt();
  if (( gasDec >= gasM) || (smokeDec >= smokeM)) {
    digitalWrite(BUZZER, HIGH);
    tOff = 1;
  }
  else if (alm_stt == 1) {      //Night
    if ((motion2 == "detected") || (door == "open")) {
      digitalWrite(BUZZER, HIGH);
      tOff = 1;
    }
  }
  else if (alm_stt == 2) {     //Away
    if ((motion2 == "detected") || (motion1 == "detected") || (door == "open")) {
      digitalWrite(BUZZER, HIGH);
      tOff = 1;
    }
  }
  else if (alm_stt == 3) {    //Stay Baby
    if ((tempDec <= tempMin) || (tempDec >= tempMax)) {
      digitalWrite(BUZZER, HIGH);
      tOff = 1;
    }
  }
  else if (alm_stt == 4) {    //Night Baby
    if ((tempDec <= tempMin) || (tempDec >= tempMax) || (motion2 == "detected") || (door == "open")) {
      digitalWrite(BUZZER, HIGH);
      tOff = 1;
    }
  }
  chk = 0;
}

void almLim(void) {
  tempMin = readEEPROM(EEPR, 2);
  tempMax = readEEPROM(EEPR, 3);
  gasM = readEEPROM(EEPR, 4);
  smokeM = readEEPROM(EEPR, 5);
}

void setup() {
  Serial.begin(115200);
  Serial1.begin(115200);
  SPI.begin();
  mfrc522.PCD_Init();
  Wire.begin();
  tft.begin();
  tft.setRotation(iliRotation270);  // landscape
  myTouch.InitTouch();
  myTouch.setPrecision(PREC_MEDIUM);
  tft.setFont(Arial_bold_14);
  alm_stt = readEEPROM(EEPR, 0);
  drawLcd1();
  color_t = ILI9341_WHITE;
  color_b = ILI9341_BLUE;
  color_p = readEEPROM(EEPR, 1);
  colors();
  almLim();
  pinMode(BUZZER, OUTPUT);
  pinMode(4, OUTPUT);
  digitalWrite(4, LOW);
}

void loop() {
  if (myTouch.dataAvailable())
  {
    myTouch.read();
    uint16_t x = myTouch.getX();
    uint16_t y = myTouch.getY();

    if (lcd_p == 1) {
      drawLcd2();
      delay(20);
    }
    else if (lcd_p == 2) {
      if ((y >= 55) && (y <= 185)) {
        if ((x >= 20) && (x <= 150))
        {
          waitForIt(20, 55, 130, 130, 10);
          drawLcd3();
        }
        if ((x >= 170) && (x <= 300))
        {
          waitForIt(170, 55, 130, 130, 10);
          drawLcd5();
        }
      }
      if ((y >= 210) && (y <= 240) && (x >= 0) && (x <= 30)) {
        waitForIt(0, 210, 30, 30, 10);
        drawLcd1();
      }
    }
    else if (lcd_p == 3) {
      if ((y >= 55) && (y <= 185) && (x >= 170) && (x <= 300)) {
        waitForIt(170, 55, 130, 130, 10);
        drawLcd4();
      }
      if ((y >= 210) && (y <= 240) && (x >= 0) && (x <= 30)) {
        waitForIt(0, 210, 30, 30, 10);
        drawLcd2();
      }
    }
    else if (lcd_p == 4) {
      if ((y >= 210) && (y <= 240) && (x >= 0) && (x <= 30)) {
        waitForIt(0, 210, 30, 30, 10);
        drawLcd3();
      }
    }
    else if (lcd_p == 5) {
      if ((y >= 210) && (y <= 240) && (x >= 0) && (x <= 30)) {
        waitForIt(0, 210, 30, 30, 10);
        drawLcd2();
      }
    }
    else if (lcd_p == 6) {
      if ((y >= 40) && (y <= 120)) {
        if ((x >= 70) && (x <= 150))
        {
          waitForIt(70, 40, 80, 80, 10);
          drawLcd7();
        }
        if ((x >= 190) && (x <= 270))
        {
          waitForIt(190, 40, 80, 80, 10);
          drawLcd8();
        }
      }
      if ((y >= 140) && (y <= 220)) {
        if ((x >= 70) && (x <= 150))
        {
          waitForIt(70, 140, 80, 80, 10);
          drawLcd9();
        }
        if ((x >= 190) && (x <= 270))
        {
          waitForIt(190, 140, 80, 80, 10);
          drawLcd10();
        }
      }
      if ((y >= 210) && (y <= 240) && (x >= 0) && (x <= 30)) {
        waitForIt(0, 210, 30, 30, 10);
        drawLcd2();
      }
    }
    else if (lcd_p == 7) {
      if ((y >= 50) && (y <= 130)) {
        if ((x >= 20) && (x <= 100))
        {
          waitForIt(20, 50, 80, 80, 10);
          alm_stt = 0; //stay
          drawLcd1();
        }
        if ((x >= 120) && (x <= 200))
        {
          waitForIt(120, 50, 80, 80, 10);
          alm_stt = 1; //night
          drawLcd1();
          runTime();
        }
        if ((x >= 220) && (x <= 300))
        {
          waitForIt(220, 50, 80, 80, 10);
          alm_stt = 2; //away
          drawLcd1();
          runTime();
        }
      }
      if ((y >= 150) && (y <= 230)) {
        if ((x >= 70) && (x <= 150))
        {
          waitForIt(70, 150, 80, 80, 10);
          alm_stt = 3; //stay baby
          drawLcd1();
        }
        if ((x >= 170) && (x <= 250))
        {
          waitForIt(170, 150, 80, 80, 10);
          alm_stt = 4; //night baby
          drawLcd1();
          runTime();
        }
      }
      if ((y >= 210) && (y <= 240) && (x >= 0) && (x <= 30)) {
        waitForIt(0, 210, 30, 30, 10);
        drawLcd2();
      }
      writeEEPROM(EEPR, 0, alm_stt);
    }
    else if (lcd_p == 8) {
      if ((y >= 40) && (y <= 90))    {
        if ((x >= 24) && (x <= 74))
        {
          waitForIt(24, 40, 50, 50, 10);
          ncolor_p = 1;
        }
        if ((x >= 98) && (x <= 148))
        {
          waitForIt(98, 40, 50, 50, 10);
          ncolor_p = 2;
        }
        if ((x >= 172) && (x <= 222))
        {
          waitForIt(172, 40, 50, 50, 10);
          ncolor_p = 3;
        }
        if ((x >= 246) && (x <= 296))
        {
          waitForIt(246, 40, 50, 50, 10);
          ncolor_p = 4;
        }
      }
      if ((y >= 110) && (y <= 160))    {
        if ((x >= 24) && (x <= 74))
        {
          waitForIt(24, 110, 50, 50, 10);
          ncolor_p = 5;
        }
        if ((x >= 98) && (x <= 148))
        {
          waitForIt(98, 110, 50, 50, 10);
          ncolor_p = 6;
        }
        if ((x >= 172) && (x <= 222))
        {
          waitForIt(172, 110, 50, 50, 10);
          ncolor_p = 7;
        }
        if ((x >= 246) && (x <= 296))
        {
          waitForIt(246, 110, 50, 50, 10);
          ncolor_p = 8;
        }
      }
      if ((y >= 180) && (y <= 230))    {
        if ((x >= 98) && (x <= 148))
        {
          waitForIt(98, 180, 50, 50, 10);
          ncolor_p = 9;
        }
        if ((x >= 172) && (x <= 222))
        {
          waitForIt(172, 180, 50, 50, 10);
          ncolor_p = 10;
        }
        if ((x >= 246) && (x <= 296))
        {
          waitForIt(246, 180, 50, 50, 10);
          ncolor_p = 11;
        }
      }
      if ((y >= 210) && (y <= 240) && (x >= 0) && (x <= 30)) {
        waitForIt(0, 210, 30, 30, 10);
        drawLcd6();
      }
      if (ncolor_p != color_p) {
        color_p = ncolor_p;
        writeEEPROM(EEPR, 1, color_p);
        colors();
        drawLcd8();
      }
    }
    else if ((lcd_p == 9) || (lcd_p == 10)) {
      if ((y >= 210) && (y <= 240) && (x >= 0) && (x <= 30)) {
        waitForIt(0, 210, 30, 30, 10);
        drawLcd6();
      }
    }
    else if (lcd_p == 11) {
      if ((y >= 55) && (y <= 185))    {
        if ((x >= 20) && (x <= 150))
        {
          waitForIt(20, 55, 130, 130, 10);
          drawLcd12();
        }
        if ((x >= 170) && (x <= 300))
        {
          waitForIt(170, 55, 130, 130, 10);
          drawLcd14();
        }
      }
      if ((y >= 210) && (y <= 240) && (x >= 0) && (x <= 30)) {
        waitForIt(0, 210, 30, 30, 10);
        drawLcd2();
      }
    }
    else if (lcd_p == 12) {
      if ((y >= 50) && (y <= 130))    {
        if ((x >= 20) && (x <= 100))
        {
          waitForIt(20, 50, 80, 80, 10);
          qqq = "Temp Min";
          drawLcd13();
        }
        if ((x >= 120) && (x <= 200))
        {
          waitForIt(120, 50, 80, 80, 10);
          qqq = "Gas";
          drawLcd13();
        }
        if ((x >= 220) && (x <= 300))
        {
          waitForIt(220, 50, 80, 80, 10);
          qqq = "Enter Hour";
          drawLcd13();
        }
      }
      if ((y >= 150) && (y <= 230))    {
        if ((x >= 70) && (x <= 150))
        {
          waitForIt(70, 150, 80, 80, 10);
          qqq = "Smoke";
          drawLcd13();
        }
        if ((x >= 170) && (x <= 250))
        {
          waitForIt(170, 150, 80, 80, 10);
          qqq = "year";
          drawLcd13();
        }
      }
      if ((y >= 210) && (y <= 240) && (x >= 0) && (x <= 30)) {
        waitForIt(0, 210, 30, 30, 10);
        drawLcd11();
      }
    }
    else if (lcd_p == 13) {
      if ((y >= 25) && (y <= 77))    {
        if ((x >= 10) && (x <= 62))
        {
          waitForIt(10, 25, 52, 52, 5);
          NumbSen = NumbSen * 10 + 0;
        }
        if ((x >= 72) && (x <= 124))
        {
          waitForIt(72, 25, 52, 52, 5);
          NumbSen = NumbSen * 10 + 1;
        }
        if ((x >= 134) && (x <= 186))
        {
          waitForIt(134, 25, 52, 52, 5);
          NumbSen = NumbSen * 10 + 2;
        }
        if ((x >= 196) && (x <= 248))
        {
          waitForIt(196, 25, 52, 52, 5);
          NumbSen = NumbSen * 10 + 3;
        }
        if ((x >= 258) && (x <= 310))
        {
          waitForIt(258, 25, 52, 52, 5);
          NumbSen = NumbSen * 10 + 4;
        }
      }
      if ((y >= 87) && (y <= 139))    {
        if ((x >= 10) && (x <= 62))
        {
          waitForIt(10, 87, 52, 52, 5);
          NumbSen = NumbSen * 10 + 5;
        }
        if ((x >= 72) && (x <= 124))
        {
          waitForIt(72, 87, 52, 52, 5);
          NumbSen = NumbSen * 10 + 6;
        }
        if ((x >= 134) && (x <= 186))
        {
          waitForIt(134, 87, 52, 52, 5);
          NumbSen = NumbSen * 10 + 7;
        }
        if ((x >= 196) && (x <= 248))
        {
          waitForIt(196, 87, 52, 52, 5);
          NumbSen = NumbSen * 10 + 8;
        }
        if ((x >= 258) && (x <= 310))
        {
          waitForIt(258, 87, 52, 52, 5);
          NumbSen = NumbSen * 10 + 9;
        }
      }
      if ((y >= 149) && (y <= 201))    {
        if ((x >= 10) && (x <= 124))
        {
          waitForIt(10, 149, 114, 52, 5);
          NumbSen = 0;
          tft.fillRect(120, 220, 170, 20, color_b);
        }
        if ((x >= 196) && (x <= 310))
        {
          waitForIt(196, 149, 114, 52, 5);
          if (qqq == "Temp Min") {
            writeEEPROM(EEPR, 2, NumbSen);
            tempMin = NumbSen;
            qqq = "Temp Max";
            drawLcd13();
          }
          else if (qqq == "Temp Max") {
            writeEEPROM(EEPR, 3, NumbSen);
            tempMax = NumbSen;
            drawLcd12();
          }
          if (qqq == "Gas") {
            writeEEPROM(EEPR, 4, NumbSen);
            gasM = NumbSen;
            drawLcd12();
          }
          if (qqq == "Smoke") {
            writeEEPROM(EEPR, 5, NumbSen);
            smokeM = NumbSen;
            drawLcd12();
          }
          if (qqq == "Enter Hour") {
            Time t = rtc.time();
            Time f(t.yr, t.mon, t.date, NumbSen, t.min, t.sec, t.day);
            rtc.time(f);
            qqq = "Enter minutes";
            drawLcd13();
          }
          else if (qqq == "Enter minutes") {
            Time t = rtc.time();
            Time f(t.yr, t.mon, t.date, t.hr, NumbSen, t.sec, t.day);
            rtc.time(f);
            drawLcd12();
          }
          if (qqq == "year") {
            Time t = rtc.time();
            Time f(NumbSen, t.mon, t.date, t.hr, t.min, t.sec, t.day);
            rtc.time(f);
            qqq = "Enter month";
            drawLcd13();
          }
          else if (qqq == "Enter month") {
            Time t = rtc.time();
            Time f(t.yr, NumbSen, t.date, t.hr, t.min, t.sec, t.day);
            rtc.time(f);
            qqq = "Enter date";
            drawLcd13();
          }
          else if (qqq == "Enter date") {
            Time t = rtc.time();
            Time f(t.yr, t.mon, NumbSen, t.hr, t.min, t.sec, t.day);
            rtc.time(f);
            qqq = "Enter day 1 until 7";
            drawLcd13();
          }
          else if (qqq == "Enter day 1 until 7") {
            Time t = rtc.time();
            Time::Day day;
            switch (NumbSen) {
              case 1: day = Time::kSunday; break;
              case 2: day = Time::kMonday; break;
              case 3: day = Time::kTuesday ; break;
              case 4: day = Time::kWednesday; break;
              case 5: day = Time::kThursday; break;
              case 6: day = Time::kFriday; break;
              case 7: day = Time::kSaturday;
            }
            Time f(t.yr, t.mon, t.date, t.hr, t.min, t.sec, day);
            rtc.time(f);
            drawLcd12();
          }
          NumbSen = 0;
        }
      }
      if (NumbSen != 0)tft.printAt(String(NumbSen), 120, 220);
      if ((y >= 210) && (y <= 240) && (x >= 0) && (x <= 30)) {
        waitForIt(0, 210, 30, 30, 10);
        NumbSen = 0;
        drawLcd12();
      }
    }
    else if (lcd_p == 14) {
      if ((y >= 55) && (y <= 185)) {
        if ((x >= 20) && (x <= 150))
        {
          waitForIt(20, 55, 130, 130, 10);
          digitalWrite(4, HIGH);
          drawLcd15();
          delay(2000);
          digitalWrite(4, LOW);
        }
        if ((x >= 170) && (x <= 300))
        {
          digitalWrite(BUZZER, HIGH);
          waitForIt(170, 55, 130, 130, 10);
          digitalWrite(BUZZER, LOW);
        }
      }
      if ((y >= 210) && (y <= 240) && (x >= 0) && (x <= 30)) {
        waitForIt(0, 210, 30, 30, 10);
        drawLcd11();
      }
    }
    else if (lcd_p == 15) {
      if ((y >= 210) && (y <= 240) && (x >= 0) && (x <= 30)) {
        waitForIt(0, 210, 30, 30, 10);
        drawLcd14();
      }
    }
  }//finish touch

  if (lcd_p == 1) {
    Time t = rtc.time();
    if (t.sec == 0) {
      drawLcd1();
      delay(1000);
    }
  }

  else if (lcd_p == 5) {
    if (readUIDchip()) {
      //read leader card from eeprom
      uint8_t rc[4];
      for (i = 0; i < 4; i++) {
        rc[i] = readEEPROM(EEPR, 10 + i);
      }
      if ((rc[0] == readCard[0]) && (rc[1] == readCard[1]) && (rc[2] == readCard[2]) && (rc[3] == readCard[3])) drawLcd11();
      else {
        if (cheak_chip()) {
          drawLcd6();
          err = 0;
        }
        if (lcd_p != 6) {
          err++;
          drawLcd5();
          digitalWrite(BUZZER, HIGH);
          delay(500);
          if (err == 3) {
            tOff = 1;
            digitalWrite(BUZZER, HIGH);
          }
          else {
            digitalWrite(BUZZER, LOW);
            delay(1500);//delay for user
          }
        }
      }
    }
  }

  else if (lcd_p == 9)
    add_chip();

  else if (lcd_p == 10)
    del_chip();

  if (chk == 1) {
    checkSensor();
    if (lcd_p == 1)
      checkBatt();
  }

  if (tOff == 1) {
    if (readUIDchip()) {
      if (cheak_chip()) {
        digitalWrite(BUZZER, LOW);
        tOff = 0;
      }
    }
  }
}

void serialEvent1() {
  while (Serial1.available() > 0) {
    c = Serial1.read();
    ser += c;
  }

  for (topic = "", i = 0; i <= ser.length() ; i++) {
    topic += ser[i];

    if (topic == "g") {
      gas = "";
      for (i++; i <= ser.length(); i++)
        gas += ser[i];
      ser = "";
    }
    if (topic == "bg") {
      gasBatt = "";
      for (i++; i <= ser.length(); i++)
        gasBatt += ser[i];
      ser = "";
    }
    if (topic == "s") {
      smoke = "";
      for (i++; i <= ser.length(); i++)
        smoke += ser[i];
      ser = "";
    }
    if (topic == "bs") {
      smokeBatt = "";
      for (i++; i <= ser.length(); i++)
        smokeBatt += ser[i];
      ser = "";
    }
    if (topic == "m1") {
      motion1 = "";
      for (i++; i <= ser.length(); i++)
        motion1 += ser[i];
      ser = "";
      if (motion1 == "1")motion1 = "detected";
      else if (motion1 == "0")motion1 = "not detect";
    }
    if (topic == "bm1") {
      motion1Batt = "";
      for (i++; i <= ser.length(); i++)
        motion1Batt += ser[i];
      ser = "";
    }
    if (topic == "m2") {
      motion2 = "";
      for (i++; i <= ser.length(); i++)
        motion2 += ser[i];
      ser = "";
      if (motion2 == "1")motion2 = "detected";
      else if (motion2 == "0")motion2 = "not detect";
    }
    if (topic == "bm2") {
      motion2Batt = "";
      for (i++; i <= ser.length(); i++)
        motion2Batt += ser[i];
      ser = "";
    }
    if (topic == "t") {
      temp = "";
      for (i++; i <= ser.length(); i++)
        temp += ser[i];
      ser = "";
    }
    if (topic == "bt") {
      tempBatt = "";
      for (i++; i <= ser.length(); i++)
        tempBatt += ser[i];
      ser = "";
    }
    if (topic == "d") {
      door = "";
      for (i++; i <= ser.length(); i++)
        door += ser[i];
      ser = "";
    }
    if (topic == "bd") {
      doorBatt = "";
      for (i++; i <= ser.length(); i++)
        doorBatt += ser[i];
      ser = "";
    }
    if (topic == "c") {
      checkA = "";
      for (i++; i <= ser.length(); i++)
        checkA += ser[i];
      if (checkA == "1")ut = 1;
      else if (checkA == "2")us = 1;
      else if (checkA == "3")ug = 1;
      else if (checkA == "4")ud = 1;
      else if (checkA == "5")um1 = 1;
      else if (checkA == "6")um2 = 1;
      else if (checkA == "n1")ut = 2;
      else if (checkA == "n2")us = 2;
      else if (checkA == "n3")ug = 2;
      else if (checkA == "n4")ud = 2;
      else if (checkA == "n5")um1 = 2;
      else if (checkA == "n6")um2 = 2;
      ser = "";
    }
    if ((ser[0] != 'g') && (ser[0] != 'b') && (ser[0] != 's') && (ser[0] != 'm') && (ser[0] != 't') && (ser[0] != 'd') && (ser[0] != 'c')) {
      ser = "";
    }
    chk = 1;
  }
  if (lcd_p == 3)drawLcd3();
  if (lcd_p == 4)drawLcd4();
  if (lcd_p == 15)drawLcd15();
}
