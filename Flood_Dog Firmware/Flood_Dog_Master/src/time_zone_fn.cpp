#include "time_zone_fn.h"
#include "sys_status.h"
#include "particle_fn.h"

/**
 * @brief Sets the time zone by offset from UTC.
 * 
 * @details Extracts the integer offset from the string passed in, makes sures it's a valid input,
 * sets the appropriate variables, and publishes the offset to the monitoring system.
 *
 * @param command A string with the offset from UTC that indicats the offset. Only values of [-12]-[12] are accepted.
 * Values outside this range will cause the function to return 0 to indicate an invalid entry.
 * 
 * @return 1 if successful, 0 if invalid command
 */
int setTimeZone(String command)
{
  char * pEND;
  char data[256];
  Particle.syncTime();                                                        // Set the clock each day
  waitFor(Particle.syncTimeDone,30000);                                       // Wait for up to 30 seconds for the SyncTime to complete
  int8_t tempTimeZoneOffset = strtol(command,&pEND,10);                       // Looks for the first integer and interprets it
  if ((tempTimeZoneOffset < -12) | (tempTimeZoneOffset > 12)) return 0;       // Make sure it falls in a valid range or send a "fail" result
  sysStatus.timezone = (float)tempTimeZoneOffset;
  Time.zone(sysStatus.timezone);
  systemStatusWriteNeeded = true;                                             // Need to store to FRAM back in the main loop
  snprintf(currentOffsetStr,sizeof(currentOffsetStr),"%2.1f UTC",(Time.local() - Time.now()) / 3600.0);
  if (Particle.connected()) {
    snprintf(data, sizeof(data), "Time zone offset %i",tempTimeZoneOffset);
    Particle.publish("Time",data, PRIVATE);
    waitUntil(meterParticlePublish);
    Particle.publish("Time",Time.timeStr(Time.now()), PRIVATE);
  }
  return 1;
}

/**
 * @brief Applies the DST offset when appropriate.
 * 
 * @details Extracts the integer offset from the string passed in, determines if DST needs to be applied to the clock,
 * and communicates the action taken back to the remote monitoring service.
 *
 * @param command A string with the number of hours that will be added for DST. Only values of 0-2 are accepted.
 * Values outside this range will cause the function to return 0 to indicate an invalid entry.
 * 
 * @return 1 if successful, 0 if invalid command
 */
int setDSTOffset(String command) {                                      // This is the number of hours that will be added for Daylight Savings Time 0 (off) - 2
  char * pEND;
  char data[256];
  time_t t = Time.now();
  int8_t tempDSTOffset = strtol(command,&pEND,10);                      // Looks for the first integer and interprets it
  if ((tempDSTOffset < 0) | (tempDSTOffset > 2)) return 0;              // Make sure it falls in a valid range or send a "fail" result
  Time.setDSTOffset((float)tempDSTOffset);                              // Set the DST Offset
  sysStatus.dstOffset = (float)tempDSTOffset;
  systemStatusWriteNeeded = true;
  snprintf(data, sizeof(data), "DST offset %2.1f",sysStatus.dstOffset);
  if (Time.isValid()) isDSTusa() ? Time.beginDST() : Time.endDST();     // Perform the DST calculation here
  
  snprintf(currentOffsetStr,sizeof(currentOffsetStr),"%2.1f UTC",(Time.local() - Time.now()) / 3600.0);
  if (Particle.connected()) {
    Particle.publish("Time",data, PRIVATE);
    waitUntil(meterParticlePublish);
    Particle.publish("Time",Time.timeStr(t), PRIVATE);
  }
  return 1;
}

/**
 * @brief Determines whether or not USA is currently observing Daylight Savings Time or Standard Time
 *
 * @details United States of America Summer Timer calculation (2am Local Time - 2nd Sunday in March/ 1st Sunday in November)
 * Adapted from @ScruffR's code posted here https://community.particle.io/t/daylight-savings-problem/38424/4
 * The code works in from months, days and hours in succession toward the two transitions
 *
 * @return true if currently observing DST, false if observing standard time
 */
bool isDSTusa() {
  int dayOfMonth = Time.day();
  int month = Time.month();
  int dayOfWeek = Time.weekday() - 1; // make Sunday 0 .. Saturday 6

  // By Month - inside or outside the DST window
  if (month >= 4 && month <= 10)
  { // April to October definetly DST
    return true;
  }
  else if (month < 3 || month > 11)
  { // before March or after October is definetly standard time
    return false;
  }

  boolean beforeFirstSunday = (dayOfMonth - dayOfWeek <= 0); // day of week - Sunday =0 / Saturday = 6   day of month - 1 - 31
  boolean secondSundayOrAfter = (dayOfMonth - dayOfWeek > 7);

  if (beforeFirstSunday && !secondSundayOrAfter) return (month == 11);
  else if (!beforeFirstSunday && !secondSundayOrAfter) return false;
  else if (!beforeFirstSunday && secondSundayOrAfter) return (month == 3);

  int secSinceMidnightLocal = Time.now() % 86400;
  boolean dayStartedAs = (month == 10); // DST in October, in March not
  // on switching Sunday we need to consider the time
  if (secSinceMidnightLocal >= 2*3600)
  { // In the US, Daylight Time is based on local time
    return !dayStartedAs;
  }
  return dayStartedAs;
}

/**
 * @brief Determines whether or not New Zealand is currently in NZDT or NZST to reflect DST
 *
 * @details New Zealand Summer Timer calculation (2am Local Time - last Sunday in September/ 1st Sunday in April)
 * Adapted from @ScruffR's code posted here https://community.particle.io/t/daylight-savings-problem/38424/4
 * The code works in from months, days and hours in succession toward the two transitions
 *
 * @return true if currently observing DST, false if observing standard time
 */

/***********************************************
   This feature is not currently implemented
***********************************************/
/*

bool isDSTnz() {
  int dayOfMonth = Time.day();
  int month = Time.month();
  int dayOfWeek = Time.weekday() - 1; // make Sunday 0 .. Saturday 6

  // By Month - inside or outside the DST window - 10 out of 12 months with April and September in question
  if (month >= 10 || month <= 3)
  { // October to March is definetly DST - 6 months
    return true;
  }
  else if (month < 9 && month > 4)
  { // before September and after April is definetly standard time - - 4 months
    return false;
  }

  boolean beforeFirstSunday = (dayOfMonth - dayOfWeek < 6);
  boolean lastSundayOrAfter = (dayOfMonth - dayOfWeek > 23);

  if (beforeFirstSunday && !lastSundayOrAfter) return (month == 4);
  else if (!beforeFirstSunday && !lastSundayOrAfter) return false;
  else if (!beforeFirstSunday && lastSundayOrAfter) return (month == 9);

  int secSinceMidnightLocal = Time.now() % 86400;
  boolean dayStartedAs = (month == 10); // DST in October, in March not
  // on switching Sunday we need to consider the time
  if (secSinceMidnightLocal >= 2*3600)
  { // Daylight Time is based on local time
    return !dayStartedAs;
  }
  return dayStartedAs;
}
*/
