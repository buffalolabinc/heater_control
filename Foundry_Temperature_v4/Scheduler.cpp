#include "Common.h"

enum states {off, daytime, nighttime, weekend };  //override turns heat up to daytime setpoint for 2 hours.

int currentSetpoint;
int currentState = off;

//#define DAY_START_HOUR 7   // day starts at 7:30 AM
//#define DAY_START_MIN 30   // day starts at 7:30 AM
//#define DAY_END_HOUR 18    // day ends at 6:00 PM
//#define DAY_END_MIN 00    // day ends at 6:00 PM
//
//#define DAY_START ((DAY_START_HOUR * 100) + DAY_START_MIN)
//#define DAY_END ((DAY_END_HOUR * 100) + DAY_END_MIN)

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
      currentSetpoint = GetNightSetpoint();
    currentState = weekend;
    currentAlarm = Alarm.alarmOnce(0, 0, 1, UpdateSchedule);  //Set an alarm to expire  at midnight
    Serial.print("Weekend Schedule. current setpoint is "); Serial.println(currentSetpoint);
    Serial.print("   current alarm is "); Serial.println(dtINVALID_ALARM_ID != currentAlarm?"valid":"invalid");
  }
  else
  {
    if ((currentHour >= GetDayStart()) && (currentHour <  GetDayEnd()))
    {
      if (!overrideEnabled)
        currentSetpoint = GetDaySetpoint();
      currentState = daytime;
      currentAlarm = Alarm.alarmOnce(GetDayEnd() / 100, GetDayEnd() % 100, 1, UpdateSchedule);  //Set an alarm to expire  at DAY_END
      Serial.print("Daytime Schedule. current setpoint is "); Serial.println(currentSetpoint);
      Serial.print("   current alarm is "); Serial.println(dtINVALID_ALARM_ID != currentAlarm?"valid":"invalid");
   }
    else
    {
      if (!overrideEnabled)
        currentSetpoint = GetNightSetpoint();
      currentState = nighttime;
      currentAlarm = Alarm.alarmOnce(GetDayStart() / 100, GetDayStart() % 100, 1, UpdateSchedule);  //Set an alarm to expire  at DAY_START
      Serial.print("Nighttime Schedule. current setpoint is "); Serial.println(currentSetpoint);
      Serial.print("   current alarm is "); Serial.println(dtINVALID_ALARM_ID != currentAlarm?"valid":"invalid");
   }
  }
  LCDDisplayTemp(currentTemp, currentSetpoint);
}

void UpdateOverride()
{
  if (overrideEnabled)      
      currentSetpoint = GetOverrideSetpoint();
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
    currentSetpoint = GetDaySetpoint();
  else
    currentSetpoint = GetNightSetpoint();
  
  Serial.print("end override. current setpoint is "); Serial.println(currentSetpoint);
  Serial.print("   current alarm is "); Serial.println(dtINVALID_ALARM_ID != currentAlarm?"valid":"invalid");
}

uint32_t overrideDebounce = 0;
uint32_t lastInterrupt = 0;
uint32_t debounceCounter = 0;
void ICACHE_RAM_ATTR SetOverride()
{
  tmElements_t overrideExpiration;
  debounceCounter++;

  if (millis() - overrideDebounce > 250)
  {
    Serial.print(" debounce count: "); Serial.println(debounceCounter);
    debounceCounter = 0;
    overrideDebounce = millis();
    
    if (!overrideEnabled)
    {
      overrideEnabled = true;
      
      currentSetpoint = GetOverrideSetpoint();
     
      breakTime(now() + GetOverrideDuration(), overrideExpiration);
      overrideAlarm = Alarm.alarmOnce(overrideExpiration.Hour, overrideExpiration.Minute, overrideExpiration.Second, EndOverride);  //Set an alarm to expire exactly at the end of the override period
      Serial.print("   override alarm is "); Serial.println(dtINVALID_ALARM_ID != overrideAlarm?"valid":"invalid");
    }
    else
    {
      overrideEnabled = false;
      Alarm.free(overrideAlarm);
      if (daytime == currentState)
        currentSetpoint = GetDaySetpoint();
      else
        currentSetpoint = GetNightSetpoint();

       Serial.print("   disabling override");
    }
  }

  lastInterrupt = millis();
}
