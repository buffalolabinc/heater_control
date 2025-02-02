#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#include <OneWire.h>   //https://github.com/adafruit/MAX31850_OneWire
#include <DallasTemperature.h>   //https://github.com/adafruit/MAX31850_DallasTemp

#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

#include "Wire.h"
#include <LiquidCrystal_I2C.h> // https://github.com/fmalpartida/New-LiquidCrystal


#include "Time.h"  //https://www.pjrc.com/teensy/td_libs_Time.html
#include "TimeAlarms.h"  //https://www.pjrc.com/teensy/td_libs_TimeAlarms.html

extern char versionString[];

#define DS_PIN  2 /* ds18b20 data pin */
#define RELAY_PIN  15 /* solid state heat relay pin*/
#define OVERRIDE_BUTTON 12 /*set point schedule override button */

//#define UP_BUTTON 12 /* set point up button. Only used with adjustable thermostat */
//#define DOWN_BUTTON 13 /*set point down button. Only used with adjustable thermostat */
//#define MAX_TEMP 99   /* maximum set point. Only used with adjustable thermostat*/
//#define MIN_TEMP 50   /* minimum set point. Only used with adjustable thermostat */

extern WiFiClient client;
extern float currentTemp;   //current average temperature read from sensors
extern bool heat_on;      //heat status
extern int currentSetpoint;    //current temperature set point
extern volatile bool overrideEnabled;   //schedule override enabled

#define min(a,b) (((a) < (b)) ? (a) : (b))
#define max(a,b) (((a) > (b)) ? (a) : (b))

// WiFi functions
bool WiFiInit(void);

//NTP functions
void InitNTP();
time_t getNTPTime();
extern int cur_UTC_offset;
extern const char* days[];

//Temperature Sensor functions
void InitTempSensors();
float ReadTempSensors();
void TempStartConversion();
bool GetTempSensorAddress(uint8_t* buffer, int index);
float GetSensorTemperature(int index);


//MQTT functions
void InitAdafruitMQTT();
bool ReinitAdafruitMQTT();
bool ConnectAdafruitMQTT();
bool CheckAdafruitMQTT();
void FeedAdafruitMQTT(float fahrenheit, int setpoint, int32_t rssi);

//Daylight Savings Time functions
void CorrectDSTsettings(time_t currentUTCTime);
time_t AdvanceDSTchange(time_t dstTime);
int Check_DST(time_t currentLocalTime);
void  UpdateDST(int doChange);

//LCD display functions
void InitLCD();
void LCDUpdateDisplay();
void LCDDisplayTime(time_t localTime);
void LCDDisplayTemp(int currentTemp, int setPoint);
void LCDDisplayHeatStatus(bool onoff);
void LCDDisplayIPAddress(const char* addressString);
void LCDDisplayOverrideStatus(bool overrideEnabled);

//Setpoint scheduler functions
void InitScheduler();
void UpdateSchedule();
void UpdateOverride();
void SetOverride();

//ESP8266 Over-The-Air (OTA) uploader
void InitOTA();
void HandleOTA();

//Telnet settings handler
void TelnetInit();
void SettingsInit();
void SerialBackdoor();
void ProcessSettings();
void ListTempSensors();
String GetHostname();
char* GetMQTTServer();
int GetMQTTPort();
char* GetMQTTUser();
char* GetMQTTKey();
char* GetMQTTTempfeed();
char* GetMQTTSetfeed();
float GetDaySetpoint();
float GetNightSetpoint();
float GetOverrideSetpoint();
long GetOverrideDuration();
int GetDayStart();
int GetDayEnd();
char* GetSSID();
char* GetPassword();
