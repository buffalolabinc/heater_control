#include "Common.h"

bool heat_on = false;
AlarmID_t dstAlarm;

void setup(void) {
  Serial.begin(115200);

  pinMode(RELAY_PIN, OUTPUT);

  SettingsInit();
  SerialBackdoor();

  InitLCD();
  InitTempSensors();
  
  if (WiFiInit())   // Connect to WiFi access point.
  {
    TelnetInit();
    InitOTA();
    
    InitAdafruitMQTT();
    ConnectAdafruitMQTT();  // connect to adafruit io
    
    InitNTP();
    
    syncDST();  //initialize DST settings, start hourly alarm.
  }
  
  InitScheduler();  //initialize temperature setpoint scheduler

  CheckTemperature();
  UpdateAdafruitMQTT();
  LCDUpdateDisplay();
  
  Alarm.timerRepeat(5, CheckTemperature);       // 5 seconds, check temperature
  Alarm.timerRepeat(30, UpdateAdafruitMQTT);    // 30 seconds, Update MQTT (note - need this to be < 300 sec to keep MWTT alive).
  Alarm.timerRepeat(1, LCDUpdateDisplay);       // 1 seconds, updates the LCD time display. Also bumps the NTP syncProvider timer
  Alarm.timerRepeat(600, CheckWiFi);            // 10 minutes, checks that WiFi is still connected

//  pinMode(UP_BUTTON, INPUT_PULLUP);  //Only used with adjustable thermostat
//  pinMode(DOWN_BUTTON, INPUT_PULLUP);  //Only used with adjustable thermostat
//  attachInterrupt(digitalPinToInterrupt(UP_BUTTON), IncreaseSetPoint, FALLING);  //Only used with adjustable thermostat
//  attachInterrupt(digitalPinToInterrupt(DOWN_BUTTON), DecreaseSetPoint, FALLING);  //Only used with adjustable thermostat

  pinMode(OVERRIDE_BUTTON, INPUT_PULLUP); //see Scheduler.cpp for interrupt routine

}

void loop(void) 
{  
  Alarm.delay(50);  //alarm callbacks run in here
  yield();  //make the ESP8266 happy
  HandleOTA();
  ProcessSettings();
}

void syncDST()
{
  Serial.println("Check for DST change");
  UpdateDST(Check_DST(now()));
  
  int dstHour = (hour()+1 % 24);   //Get the next hour from now.
  //Set an alarm to expire exactly on the next hour, so we can check for DST change
  if (!Alarm.isAllocated(dstAlarm))
    dstAlarm = Alarm.alarmOnce(dstHour, 0, 0, syncDST);
  Serial.print("Next check for DST change at "); Serial.print(dstHour); Serial.print(":00"); Serial.println(" (Local)");

}

void UpdateAdafruitMQTT()
{
  if (CheckAdafruitMQTT())
    FeedAdafruitMQTT(currentTemp, currentSetpoint, WiFi.RSSI());
}

void CheckTemperature()
{
  currentTemp = ReadTempSensors();
//  uint16_t thermostat = analogRead(A0);  //fpor use with potentiometer as thermostat adjustment
//  currentSetpoint = (thermostat / 20.48) + 50; //thermostat range is 50F..100F
  
//  Serial.print("thermostat = "); Serial.println(thermostat);
  Serial.print("set point = "); Serial.println(currentSetpoint);

  if (currentTemp >= (currentSetpoint + 1.5))
  {
    digitalWrite(RELAY_PIN, LOW);
    heat_on = false;
    Serial.println("heat off");
  }
  if (currentTemp <= (currentSetpoint - 1.5))
  {
    digitalWrite(RELAY_PIN, HIGH);
    heat_on = true;
    Serial.println("heat on");
  }
  LCDDisplayHeatStatus(heat_on);

  LCDDisplayTemp(round(currentTemp), (int)currentSetpoint);
}

void CheckWiFi()
{
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("WiFi connection lost"));
    if (WiFiInit())
    {
      TelnetInit();
      InitOTA();
      
      InitAdafruitMQTT();
      ConnectAdafruitMQTT();  // connect to adafruit io
      
      InitNTP();
      
      syncDST();  //initialize DST settings, start hourly alarm.
    }
  }
  LCDDisplayIPAddress(WiFi.localIP().toString().c_str());
}
