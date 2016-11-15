// include the library code:
#include "Wire.h"
//#include "Adafruit_LiquidCrystal.h"
#include <LiquidCrystal_I2C.h>  // F Malpartida's NewLiquidCrystal library

#include "Common.h"

// Connect via i2c, default address #0 (A0-A2 not jumpered)
//Adafruit_LiquidCrystal lcd(0);
#define I2C_ADDR    0x3F  // Define I2C Address for the PCF8574A 
#define BACKLIGHT_PIN  3
#define EN_PIN  2
#define RW_PIN  1
#define RS_PIN  0
#define D4_PIN  4
#define D5_PIN  5
#define D6_PIN  6
#define D7_PIN  7

#define LED_OFF 0
#define LED_ON  1

LiquidCrystal_I2C  lcd(I2C_ADDR, EN_PIN, RW_PIN, RS_PIN, D4_PIN, D5_PIN, D6_PIN, D7_PIN);

char* days[] = {"", "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

bool activityBlinker = true;

void InitLCD() 
{
  // set up the LCD's number of rows and columns: 
//  lcd.begin(16, 2);
  lcd.begin(20, 4);
  lcd.setBacklightPin(BACKLIGHT_PIN, POSITIVE);
  lcd.setBacklight(LED_ON);
}

//void LCDDisplayTime(time_t utcTime, int utcOffset)
void LCDDisplayTime(time_t localTime)
{
  tmElements_t tm;
  char timeBuf[21];

//  utcTime += (utcOffset * 3600);
//  breakTime(utcTime, tm);
  breakTime(localTime, tm);
 // snprintf(timeBuf, 21, "%02i:%02i:%02i  %02i/%02i/%4i", tm.Hour, tm.Minute, tm.Second, tm.Month, tm.Day, tm.Year + 1970);
  if (activityBlinker) // blink colon in time
    snprintf(timeBuf, 21, "%02i:%02i %3s %02i/%02i/%4i", tm.Hour, tm.Minute, days[tm.Wday], tm.Month, tm.Day, tm.Year + 1970);
  else
    snprintf(timeBuf, 21, "%02i %02i %3s %02i/%02i/%4i", tm.Hour, tm.Minute, days[tm.Wday], tm.Month, tm.Day, tm.Year + 1970);
  activityBlinker = !activityBlinker;
  
  lcd.setCursor(0, 0);
  lcd.print(timeBuf);
}

void LCDDisplayTemp(int currentTemp, int setPoint)
{
  char tempBuf[17];

  snprintf(tempBuf, 17, "Temp %3i  Set %2i", currentTemp, setPoint);
  lcd.setCursor(0, 1);
  lcd.print(tempBuf);
}

void LCDDisplayHeatStatus(bool onoff)
{
  lcd.setCursor(17, 3);
  lcd.print(onoff ? " on" : "off");
}

void LCDDisplayIPAddress(const char* addressString)
{
  lcd.setCursor(0, 3);
  lcd.print(addressString);
}

