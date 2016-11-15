#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#include <OneWire.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

#include "Time.h"
#include "TimeAlarms.h"

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

#define min(a,b) (((a) < (b)) ? (a) : (b))
#define max(a,b) (((a) > (b)) ? (a) : (b))

// WiFi functions
void WiFiInit(void);

//NTP functions
void InitNTP();
time_t getNTPTime();
extern int cur_UTC_offset;

//Temperature Sensor functions
void InitTempSensors();
float ReadTempSensors();

//MQTT functions
void InitAdafruitMQTT();
void ConnectAdafruitMQTT();
void CheckAdafruitMQTT();
void FeedAdafruitMQTT(float fahrenheit);

//Daylight Savings Time functions
void CorrectDSTsettings(time_t currentTime);
time_t AdvanceDSTchange(time_t dstTime);
int Check_DST(time_t currentTime);
void  UpdateDST(int doChange);

//LCD display functions
void InitLCD();
//void LCDDisplayTime(time_t utcTime, int utcOffset);
void LCDDisplayTime(time_t localTime);
void LCDDisplayTemp(int currentTemp, int setPoint);
void LCDDisplayHeatStatus(bool onoff);
void LCDDisplayIPAddress(const char* addressString);

//Setpoint scheduler functions
void InitScheduler();
void UpdateSchedule();
void SetOverride();

//ESP8266 Over-The-Air (OTA) uploader
void InitOTA();
void HandleOTA();
