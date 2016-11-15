/////////////////////////////////////////////////////////////////////////////////////////
//
//  Daylight Savings Time
//
/////////////////////////////////////////////////////////////////////////////////////////

#include "Common.h"

#define CLOCK_DEBUG

//Set the next two DST changes for your locality in Unix time. Note - these time stamps must be UTC, 
//  regardless of your location.  The timestamps here are for US.
// Also note : you don't ever need to update these again. The code will automatically figure out
// the correct times every time it starts up, using these two static references.
//You can use this converter to get the unix time: http://www.onlineconversion.com/unix_time.htm
time_t dst_start_seconds  = 1489284000; //Mar 12, 2017 2:00:00 EST
time_t dst_end_seconds    = 1478397600; //Nov  6, 2016 2:00:00 EDST

//Set your local UTC offsets here.  Once set correctly, no need to change these again. 
long dst_UTC_offset = -4 * 3600;  //Daylight Savings time UTC offset, US Eastern Daylight Time           
long std_UTC_offset = -5 * 3600;  //Standard Time UTC offset, US Eastern Standard Time

//  Code will figure out correct current offset every time it starts up
int cur_UTC_offset;  //current UTC offset

char* FormatFullTime(time_t t, char* buf, int bufLen)
{
  tmElements_t tm;
  breakTime(t, tm);
  snprintf(buf, bufLen, "%02i/%02i/%4i %02i:%02i:%02i", tm.Month, tm.Day, tm.Year+1970, tm.Hour, tm.Minute, tm.Second);
  return buf;
}

//This routine figures out when the dst change happens next year.
//Input is time of dst change this year.
//NOTE: overwrites input dstTime with new dstTime, and returns pointer.
time_t AdvanceDSTchange(time_t dstTime) 
{
  int        nextYear;
  int        thisYear;

  dstTime += (time_t)((364L * 24) * 3600);        //add 364 days to get same day next year
                                                  // no - really.  We'll take care of the extra day
                                                  // later.  And leap-years don't matter.  Honest!
  
  //OK, so we need to add a week every now and then, beacause we're only incrementing by 364 days.
  // For instance, in the US, DST starts on the second Sunday in March. 
  // So, for example, starting in 2004, the date progression for the second sunday is as follows:
  //    3/14/04    (leap)
  //    3/13/05
  //    3/12/06
  //    3/10/07
  //    3/09/08    (leap)
  //    3/08/09
  //    3/14/10
  //    3/13/11
  //    3/11/12    (leap)
  //
  // Notice that the day of the month decrements every year, except in leap years, in which
  // it decrements by two.  We don't want to decrement past the 8th, since  that would take
  // us to the FIRST sunday of the month.  So, we need to add a week.  This is what happens
  // in 2010 - the second sunday is the 14th of the month, not the seventh.
  // Leap-years goof up what would otherwise be a simple seven-year progression, 
  // so we can't just add a week every seven years.
  //
  // Sometimes, we only get one leap-year in the seven-day rotation of the week.  In these cases,
  // we want to add the week after six years.  Sometimes, though, we get two leap-years in the rotation,
  // and in these cases we want to add a week after only 5 years.  Is 5 enough?  Are 6 too many?
  // What's an algorithm to do?!
  //
  // Well, it turns out that there is a regular 28-year pattern to this: 6-5-6-5-6 6-5-6-5-6 etc.
  // That is, we add the week after 6 years, then after 5 more, then after 6 more, then after 5 more, etc.
  // Note that when the pattern repeats, we have two 6-year intervals in a row, so it's not just
  // a straight alternation of 5-year and 6-year intervals.
  //
  // Now, we could have come up with some elaborate table-driven approach to this. 
  // Take the year modulo 28, and look it up in a table to see if we want to add a week or not.
  // Or we can do it with arithmetic.  We only call this twice a year, so toss a coin.
  // Anyway, the 6-5-6-5-6 progression reminded me of the kinds of things that come out of 
  // line-drawing algorithms. For a slope of 1-in-6 (slope = 0.16666), you'd draw six pixels 
  // on the x-axis, then move one pixel on the y-axis, draw six more pixels on the x-axis, and so on.
  // Our interval progression turns out to be a slope of exactly 1-in-5.6 (or slope = 0.1785)
  //
  // Line drawing algorithms work by accumulating fractional components until they overflow into
  // an integer.  Add 0.6666... six times, and you finally get an integer component of 1, and that's when
  // you increment the y-axis.
  // Our approach is essentially the same. We divide the current year and the next year each by 5.6, and if
  // if the integer parts of the two dividends differ by 1, we have to add a week:
  //    weeks to add = (int) (year+1 / 5.6) - (int)(year / 5.6)    -- result will either be 0 or 1
  //
  // The only fly in this ointment is that we hit the 28-year pattern in the wrong place if we
  // simply use the value of the current year.  We have to bias the current year by subtracting 16 
  // (or adding 12) to get synchronized with the pattern.
  //
  // Ummm, two - TWO flies in this ointment.  The other fly is that leap-years do not occur in century years,
  // like 1900 and 2100  (2000, of course, had to be different, and WAS a leap-year).  Anyway, believe
  // it or not, our NTP clock will go past 2100. So what the hell, we might as well make this work in the
  // next century. All it means is that we change our pattern bias from -16 to -22 (or from +12 to +6, if
  // you prefer.  We've always cultivated a negative attitude, so we will be subtracting).

  // first, call gmtime to find out what year it is next year.
  nextYear = year(dstTime);              //we already added 364 days to dstTime

  if (nextYear < 2100)                     
    nextYear -= 16;                        //get us in synch with the 28-year pattern
  else
    nextYear -= 22;                        //alright, account for that pesky century

  //integer arithmetic instead of floating point ... (int)(year / 5.6) == ((year * 10) / 56)
  nextYear *= 10;                          // next year * 10
  thisYear = nextYear - 10;                // this year = (next year - 1) * 10

  nextYear /= 56;                          //OK, so now we divide by 56 to get our integer result
  thisYear /= 56;

  if (nextYear - thisYear)          //if integer parts differ by 1, we add a week
  {
    dstTime += (time_t)((7L * 24) * 3600);    //add a week.
  }

  return dstTime;
}


//Our DST settings can be wrong when we first start up.  
//This routine makes sure that everything is set correctly.
void CorrectDSTsettings(time_t currentTime) 
{
  #ifdef CLOCK_DEBUG
  char buf[32];
  Serial.print("CorrectDSTsettings: currently (UTC) "); Serial.println(FormatFullTime(currentTime, buf, 32)); 
  #endif

  //First, make sure we're not behind the times.  DST change times should ALWAYS
  // be later than the current time

  #ifdef CLOCK_DEBUG
  Serial.print("dst start time is (UTC) "); Serial.println(FormatFullTime(dst_start_seconds, buf, 32)); 
  #endif

  while (currentTime >= dst_start_seconds)
  {
    dst_start_seconds = AdvanceDSTchange(dst_start_seconds);
  }

  #ifdef CLOCK_DEBUG
  Serial.print("dst start time adjusted to (UTC) "); Serial.println(FormatFullTime(dst_start_seconds, buf, 32)); 
  Serial.print("dst end time is (UTC) "); Serial.println(FormatFullTime(dst_end_seconds, buf, 32)); 
  #endif

  //now check the fall change
  while (currentTime >= dst_end_seconds)
  {
    dst_end_seconds = AdvanceDSTchange(dst_end_seconds);
  }

  #ifdef CLOCK_DEBUG
  Serial.print("dst end time adjusted to (UTC) "); Serial.println(FormatFullTime(dst_end_seconds, buf, 32)); 
  Serial.println("CorrectDSTsettings Advance DST Change complete"); 
  #endif

  //Now, make sure that cur_UTC_offset points to the correct UTC offset. 
  // We have just made the adjustment of dst_start and dst_end according to the current time
  // So, if dst_start is GREATER than dst_end, then we're approaching the END of daylight savings 
  // (i.e., we're currently observing DST). This is because we will have moved dst_start to next year,
  // while dst_end is still this year. 
  // If dst start is LESS than dst end, then we're approaching the START of daylight savings 
  // (i.e., we're currently observing standard time)

  #ifdef CLOCK_DEBUG
  Serial.print("current UTC offset is "); Serial.println(cur_UTC_offset); 
  #endif

  if (dst_start_seconds >= dst_end_seconds)   //If we're currently observing daylight savings time...
  {                                                                           
    if (cur_UTC_offset != dst_UTC_offset)                
    {
      cur_UTC_offset = dst_UTC_offset;                                                                           
    }
  }
  else                                                                        //Else, we're currently observing standard time
  {
    if (cur_UTC_offset != std_UTC_offset)                // see if we're already set up correctly
    {                                                                         
      cur_UTC_offset = std_UTC_offset;                   // If not, change the current UTC offset pointer
    }
  }

  #ifdef CLOCK_DEBUG
  Serial.print("UTC offset is now "); Serial.println(cur_UTC_offset); 
  #endif

  #ifdef CLOCK_DEBUG
  Serial.println("end CorrectDSTsettings"); 
  #endif
}

int Check_DST(time_t currentTime)
{
  int doChange = 0;     //assume no change into or out of DST
 
  #ifdef CLOCK_DEBUG
  char buf[32];
  Serial.print("Check_DST: currently (UTC) "); Serial.println(FormatFullTime(currentTime, buf, 32)); 
  #endif

  //CorrectDSTsettings(currentTime);        //make sure all our daylight savings time settings are correct

  if (currentTime >= dst_start_seconds)
  {
    doChange = 1;       //spring forward
  }
  else
  {
    if (currentTime >= dst_end_seconds)
    {
      doChange = -1;       //fall back
    }
  }

  #ifdef CLOCK_DEBUG
  Serial.print("end Check_DST: doChange =  "); Serial.println(doChange); 
  #endif

  return doChange;
}

void  UpdateDST(int doChange)
{
  #ifdef CLOCK_DEBUG
  char buf[32];
  Serial.println("UpdateDST"); 
  #endif
   
  switch (doChange)
  {
    case 1:                                   //spring change to daylight savings time
      cur_UTC_offset = dst_UTC_offset;        //use DST offset
                                              // (e.g., Eastern Standard is UTC -5, Eastern daylight is UTC - 4)
      dst_start_seconds = AdvanceDSTchange(dst_start_seconds);    //advance clockInfo to next DST start time
      #ifdef CLOCK_DEBUG
      Serial.print("dst start time adjusted to (UTC) "); Serial.println(FormatFullTime(dst_start_seconds, buf, 32));
      Serial.print("current UTC offset is "); Serial.println(cur_UTC_offset); 
      #endif 
      break;

    case -1:                                  //fall change to standard time
      cur_UTC_offset = std_UTC_offset;        //use standard time offset
                                              // (e.g., Eastern Standard is UTC -5, Eastern daylight is UTC - 4)
      dst_end_seconds = AdvanceDSTchange(dst_end_seconds);      //advance clockInfo to next DST end time
      #ifdef CLOCK_DEBUG
      Serial.print("dst end time adjusted to (UTC) "); Serial.println(FormatFullTime(dst_end_seconds, buf, 32));
      Serial.print("current UTC offset is "); Serial.println(cur_UTC_offset); 
      #endif
      break;

   default :
      break;
  }
}

