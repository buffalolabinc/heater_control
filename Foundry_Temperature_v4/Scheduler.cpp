#include "Common.h"

typedef enum states {off, daytime, nighttime, weekend, override };  //override turns heat up to daytime setpoint for 2 hours.

int currentSetpoint;
int currentState = off;

#define DAY_START_HOUR 7   // day starts at 7:30 AM
#define DAY_START_MIN 30   // day starts at 7:30 AM
#define DAY_END_HOUR 18    // day ends at 6:00 PM
#define DAY_END_MIN 00    // day ends at 6:00 PM

#define DAY_START ((DAY_START_HOUR * 100) + DAY_START_MIN)
#define DAY_END ((DAY_END_HOUR * 100) + DAY_END_MIN)

#define OVERRIDE_DURATION (2 * 3600)  //Override schedule to day setpoint. Overrides last for 2 hours

#define DAY_SETPOINT 70   // daytime temperature is set to 70 degrees
#define NIGHT_SETPOINT 60  // nighttime temperature is set to 60 degrees

//these are based on Time library definition of Sunday as day 1. There is no day 0.
#define SATURDAY 7
#define SUNDAY 1
#define MONDAY 2

AlarmID_t currentAlarm;   //ID of current scheduler alarm

void UpdateSchedule()
{
  tmElements_t tm;

  breakTime(now(), tm);

  int currentDay = tm.Wday;
  int currentHour = (tm.Hour * 100) + tm.Minute;
  
  if ((currentDay == SATURDAY) || (currentDay == SUNDAY))
  {
    currentSetpoint = NIGHT_SETPOINT;
    currentState = weekend;
    currentAlarm = Alarm.alarmOnce(0, 0, 0, UpdateSchedule);  //Set an alarm to expire exactly at midnight
    Serial.println("Weekend Schedule");
  }
  else
  {
    if ((currentHour >= DAY_START) && (currentHour <  DAY_END))
    {
      currentSetpoint = DAY_SETPOINT;
      currentState = daytime;
      currentAlarm = Alarm.alarmOnce(DAY_END_HOUR, DAY_END_MIN, 0, UpdateSchedule);  //Set an alarm to expire exactly at DAY_END
      Serial.println("Daytime Schedule");
    }
    else
    {
      currentSetpoint = NIGHT_SETPOINT;
      currentState = nighttime;
      currentAlarm = Alarm.alarmOnce(DAY_START_HOUR, DAY_START_MIN, 0, UpdateSchedule);  //Set an alarm to expire exactly at DAY_START
      Serial.println("Nighttime Schedule");
    }
  }
  LCDDisplayTemp(currentTemp, currentSetpoint);
}

void InitScheduler()
{
  attachInterrupt(digitalPinToInterrupt(OVERRIDE_BUTTON), SetOverride, FALLING);
  UpdateSchedule();
}


void SetOverride()
{
  tmElements_t overrideExpiration;
  Alarm.disable(currentAlarm);
  Alarm.free(currentAlarm);
  
  currentSetpoint = DAY_SETPOINT;
  currentState = override;
 
  breakTime(now() + OVERRIDE_DURATION, overrideExpiration);
  currentAlarm = Alarm.alarmOnce(overrideExpiration.Hour, overrideExpiration.Minute, overrideExpiration.Second, UpdateSchedule);  //Set an alarm to expire exactly at the end of the override period

  LCDDisplayTemp(currentTemp, currentSetpoint);
}
