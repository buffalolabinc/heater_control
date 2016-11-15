#include "Common.h"

bool heat_on = false;

void setup(void) {
  Serial.begin(115200);

  pinMode(RELAY_PIN, OUTPUT);

  InitLCD();

  WiFiInit();   // Connect to WiFi access point.

  InitOTA();

  InitTempSensors();

  InitAdafruitMQTT();
  ConnectAdafruitMQTT();  // connect to adafruit io

  InitNTP();

  syncDST();  //initialize DST settings, start hourly alarm.

  InitScheduler();  //initialize temperature setpoint scheduler

  CheckTemperature();
  UpdateAdafruitMQTT();
  DisplayTime();
  
  Alarm.timerRepeat(15, CheckTemperature);     //check temperature every 15 seconds
  Alarm.timerRepeat(300, UpdateAdafruitMQTT); //Update MQTT every 5 minutes
  Alarm.timerRepeat(1, DisplayTime);          //updates the LCD time display. Also bumps the NTP syncProvider timer

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
}

void syncDST()
{
  Serial.println("Check for DST change");
  UpdateDST(Check_DST(now()));
  
  int dstHour = (hour()+1 % 24);   //Get the next hour from now.
  //Set an alarm to expire exactly on the next hour, so we can check for DST change
  Alarm.alarmOnce(dstHour, 0, 0, syncDST);
  Serial.print("Next check for DST change at "); Serial.print(dstHour); Serial.println(":00 UTC");

}

void UpdateAdafruitMQTT()
{
  CheckAdafruitMQTT();
  FeedAdafruitMQTT(currentTemp);
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

void DisplayTime()
{
  LCDDisplayTime(now());
}

//void IncreaseSetPoint()
//{
//  currentSetpoint = min(MAX_TEMP, setPoint + 1);
//  LCDDisplayTemp((int)currentTemp, (int)setPoint);
//}

//void DecreaseSetPoint()
//{
//  currentSetpoint = max(MIN_TEMP, setPoint - 1);
//  LCDDisplayTemp((int)currentTemp, (int)setPoint);
//}

//uint8_t debounceRead(int pin)
//{
//  uint8_t pinState = digitalRead(pin);
//  uint32_t timeout = millis();
//  while ((millis() - timeout) < 10)
//  {
//    if (digitalRead(pin) != pinState)
//    {
//      pinState = digitalRead(pin);
//      timeout = millis();
//    }
//  }
//
//  return pinState;
//}

