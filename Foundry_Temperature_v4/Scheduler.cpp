#include "Common.h"

typedef enum states {off, daytime, nighttime, weekend };  //override turns heat up to daytime setpoint for 2 hours.

int currentSetpoint;
int currentState = off;

#define DAY_START_HOUR 7   // day starts at 7:30 AM
#define DAY_START_MIN 30   // day starts at 7:30 AM
#define DAY_END_HOUR 18    // day ends at 6:00 PM
#define DAY_END_MIN 00    // day ends at 6:00 PM

#define DAY_START ((DAY_START_HOUR * 100) + DAY_START_MIN)
#define DAY_END ((DAY_END_HOUR * 100) + DAY_END_MIN)

#define OVERRIDE_DURATION (2 * 3600)  //Override schedule to day setpoint. Overrides last for 2 hours

#define DAY_SETPOINT 65   // daytime temperature is set to 65 degrees
#define NIGHT_SETPOINT 55  // nighttime temperature is set to 55 degrees

//these are based on Time library definition of Sunday as day 1. There is no day 0.
#define SATURDAY 7
#define SUNDAY 1
#define MONDAY 2

AlarmID_t currentAlarm;   //ID of current scheduler alarm
AlarmID_t overrideAlarm;   //ID of current override alarm

volatile bool overrideEnabled = false;

void UpdateSchedule()
{
  tmElements_t tm;

  breakTime(now(), tm);

  int currentDay = tm.Wday;
  int currentHour = (tm.Hour * 100) + tm.Minute;
  
  if ((currentDay == SATURDAY) || (currentDay == SUNDAY))
  {
    if (!overrideEnabled)
      currentSetpoint = NIGHT_SETPOINT;
    currentState = weekend;
    currentAlarm = Alarm.alarmOnce(0, 0, 1, UpdateSchedule);  //Set an alarm to expire  at midnight
    Serial.print("Weekend Schedule. current setpoint is "); Serial.println(currentSetpoint);
    Serial.print("   current alarm is "); Serial.println(dtINVALID_ALARM_ID != currentAlarm?"valid":"invalid");
  }
  else
  {
    if ((currentHour >= DAY_START) && (currentHour <  DAY_END))
    {
      if (!overrideEnabled)
        currentSetpoint = DAY_SETPOINT;
      currentState = daytime;
      currentAlarm = Alarm.alarmOnce(DAY_END_HOUR, DAY_END_MIN, 1, UpdateSchedule);  //Set an alarm to expire  at DAY_END
      Serial.print("Daytime Schedule. current setpoint is "); Serial.println(currentSetpoint);
      Serial.print("   current alarm is "); Serial.println(dtINVALID_ALARM_ID != currentAlarm?"valid":"invalid");
   }
    else
    {
      if (!overrideEnabled)
        currentSetpoint = NIGHT_SETPOINT;
      currentState = nighttime;
      currentAlarm = Alarm.alarmOnce(DAY_START_HOUR, DAY_START_MIN, 1, UpdateSchedule);  //Set an alarm to expire  at DAY_START
      Serial.print("Nighttime Schedule. current setpoint is "); Serial.println(currentSetpoint);
      Serial.print("   current alarm is "); Serial.println(dtINVALID_ALARM_ID != currentAlarm?"valid":"invalid");
   }
  }
  LCDDisplayTemp(currentTemp, currentSetpoint);
}

void InitScheduler()
{
  attachInterrupt(digitalPinToInterrupt(OVERRIDE_BUTTON), SetOverride, FALLING);
  UpdateSchedule();
}

void EndOverride()
{
  overrideEnabled = false;
  if (daytime == currentState)
    currentSetpoint = DAY_SETPOINT;
  else
    currentSetpoint = NIGHT_SETPOINT;
  
  Serial.print("end override. current setpoint is "); Serial.println(currentSetpoint);
  Serial.print("   current alarm is "); Serial.println(dtINVALID_ALARM_ID != currentAlarm?"valid":"invalid");
}

void SetOverride()
{
  tmElements_t overrideExpiration;

  if (!overrideEnabled)
  {
    overrideEnabled = true;
    
    currentSetpoint = DAY_SETPOINT;
   
    breakTime(now() + OVERRIDE_DURATION, overrideExpiration);
    overrideAlarm = Alarm.alarmOnce(overrideExpiration.Hour, overrideExpiration.Minute, overrideExpiration.Second, EndOverride);  //Set an alarm to expire exactly at the end of the override period
    Serial.print("   override alarm is "); Serial.println(dtINVALID_ALARM_ID != overrideAlarm?"valid":"invalid");
  }
  else
  {
    overrideEnabled = false;
    Alarm.free(overrideAlarm);
    if (daytime == currentState)
      currentSetpoint = DAY_SETPOINT;
    else
      currentSetpoint = NIGHT_SETPOINT;
  }
}
