// * The actual compiled version of this code will vary depending on
// * #defines for clusters included in the built application.
// *
// * Copyright 2014 Silicon Laboratories, Inc.                              *80*
// *******************************************************************

#include "../include/af.h"
#include "af-main.h"
#include "common.h"
#include "time-util.h"

#define mYEAR_IS_LEAP_YEAR(year)  ( (year % 4 == 0) && ((year % 100 != 0) || (year % 400 == 0)) )

PGM int8u emberAfDaysInMonth[] =
  { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

int8u emberAfGetNumberDaysInMonth( EmberAfTimeStruct *time ){
  int8u daysInMonth = 0;
  if( (time->month > 0) && (time->month < 13) ){
    daysInMonth = emberAfDaysInMonth[time->month-1];
    if( mYEAR_IS_LEAP_YEAR(time->year) && (time->month == 2) ){
      daysInMonth++;
    }
  }
  return daysInMonth;
}


void emberAfCopyDate(int8u *data, int16u index, EmberAfDate *src)
{
  data[index]   = src->year;
  data[index+1] = src->month;
  data[index+2] = src->dayOfMonth;
  data[index+3] = src->dayOfWeek;
}

int8s emberAfCompareDates(EmberAfDate* date1, EmberAfDate* date2)
{
  int32u val1 = emberAfEncodeDate(date1);
  int32u val2 = emberAfEncodeDate(date2);
  return (val1 == val2) ? 0 : ((val1 < val2) ? -1 : 1);
}

void emberAfFillTimeStructFromUtc(int32u utcTime,
                                  EmberAfTimeStruct* returnTime)
{
  boolean isLeapYear = TRUE; // 2000 was a leap year
  int32u i;
  int32u daysSince2000 = utcTime / SECONDS_IN_DAY;
  int32u secToday = utcTime - (daysSince2000 * SECONDS_IN_DAY);
  returnTime->hours = (int8u)(secToday / SECONDS_IN_HOUR);
  returnTime->minutes = (int8u)((secToday
                                - (returnTime->hours * SECONDS_IN_HOUR)) / 60);
  returnTime->seconds = (int8u)(secToday
                                - ((returnTime->hours * SECONDS_IN_HOUR)
                                   + (returnTime->minutes * 60)));
  returnTime->year = 2000;
  returnTime->month = 1;
  returnTime->day = 1;

  // march through the calendar, counting months, days and years
  // being careful to account for leapyears.
  for (i = 0; i < daysSince2000; i++) {
    int8u daysInMonth;
    if (isLeapYear && (returnTime->month == 2)) {
      daysInMonth = 29;
    } else {
      daysInMonth = emberAfDaysInMonth[returnTime->month - 1];
    }
    if (daysInMonth == returnTime->day) {
      returnTime->month++;
      returnTime->day = 1;
    } else {
      returnTime->day++;
    }
    if (returnTime->month == 13) {
      returnTime->year++;
      returnTime->month = 1;
      if (returnTime->year % 4 == 0 &&
          (returnTime->year % 100 != 0 ||
           (returnTime->year % 400 == 0)))
        isLeapYear = TRUE;
      else
        isLeapYear = FALSE;
    }
  }
}

int32u emberAfGetUtcFromTimeStruct( EmberAfTimeStruct *time ){
  // Construct a UTC timestamp given an EmberAfTimeStruct structure.
  int32u utcTime = 0;
  int16u daysThisYear;
  int32u i;

  if( (time == NULL) || (time->year < 2000) || (time->month == 0) ||
      (time->month > 12) || (time->day == 0) || (time->day > 31) ){
    return 0xFFFFFFFF;    // Invalid parameters
  }

  for( i=2000; i<time->year; i++ ){
    utcTime += ( 365 * SECONDS_IN_DAY );
    if( mYEAR_IS_LEAP_YEAR(i) ){
      utcTime += SECONDS_IN_DAY;
    }
  }
  emberAfAppPrintln("Seconds in %d years=%d", i, utcTime );
  // Utc Time now reflects seconds up to Jan 1 00:00:00 of current year.
  daysThisYear = 0;
  for( i=0; i<time->month -1 ; i++ ){
    daysThisYear += emberAfDaysInMonth[i];
    if( (i == 1) && (mYEAR_IS_LEAP_YEAR(time->year)) ){
      daysThisYear++;
    }
  }
  daysThisYear += time->day -1;
  utcTime += SECONDS_IN_DAY * daysThisYear;
  emberAfAppPrintln("daysThisYear=%d, total Sec=%d (0x%4x)", daysThisYear, utcTime, utcTime );

  // Utc Time now reflects seconds up to last completed day of current year.
  for( i=0; i<time->hours; i++ ){
    utcTime += SECONDS_IN_HOUR;
  }
  //for( i=0; i<time->minutes; i++ ){
    //iutcTime += 60;
  //}
  utcTime += (60 * time->minutes);
  utcTime += time->seconds;
  return utcTime;
}

#define SECONDS_IN_WEEK  (SECONDS_IN_DAY * 7)
// Determine which day of the week it is, from a given utc timestamp.
// Return 0=MON, 1=TUES, etc.
int8u emberAfGetWeekdayFromUtc( int32u utcTime ){
  int8u dayIndex;
  utcTime %= SECONDS_IN_WEEK;

  for( dayIndex = 0; dayIndex < 7; dayIndex++ ){
    if( utcTime < SECONDS_IN_DAY ){
      break;
    }
    utcTime -= SECONDS_IN_DAY;
  }
  // Note:  Jan 1, 2000 is a SATURDAY.
  // Do some translation work so 0=MONDAY, 5=SATURDAY, 6=SUNDAY
  if( dayIndex < 2 ){
    dayIndex += 5;
  }
  else{
    dayIndex -= 2;
  }
  return dayIndex;
}


void emberAfDecodeDate(int32u src, EmberAfDate* dest)
{
  dest->year       = (int8u)((src & 0xFF000000) >> 24);
  dest->month      = (int8u)((src & 0x00FF0000) >> 16);
  dest->dayOfMonth = (int8u)((src & 0x0000FF00) >>  8);
  dest->dayOfWeek  = (int8u) (src & 0x000000FF);
}

int32u emberAfEncodeDate(EmberAfDate* date)
{
  int32u result = ((((int32u)date->year) << 24)
                  + (((int32u)date->month) << 16)
                  + (((int32u)date->dayOfMonth) << 8)
                  + (((int32u)date->dayOfWeek)));
  return result;
}


// emberAfPrintTime expects to be passed a ZigBee time which is the number
// of seconds since the year 2000, it prints out a human readable time
// from that value.
void emberAfPrintTime(int32u utcTime)
{
#ifdef EMBER_AF_PRINT_ENABLE
  EmberAfTimeStruct time;
  emberAfFillTimeStructFromUtc(utcTime, &time);
  emberAfPrintln(emberAfPrintActiveArea,
                 "UTC time: %d/%d/%d %d:%d:%d (%4x)",
                 time.month,
                 time.day,
                 time.year,
                 time.hours,
                 time.minutes,
                 time.seconds,
                 utcTime);
#endif //EMBER_AF_PRINT_ENABLE
}

void emberAfPrintDate(const EmberAfDate * date)
{
#ifdef EMBER_AF_PRINT_ENABLE
  int32u zigbeeDate = ((((int32u)date->year) << 24)
                      + (((int32u)date->month) << 16)
                      + (((int32u)date->dayOfMonth) << 8)
                      + (((int32u)date->dayOfWeek)));

  emberAfPrint(emberAfPrintActiveArea, 
               "0x%4X (%d/%p%d/%p%d)",
               zigbeeDate,
               date->year + 1900,
               (date->month < 10 ? "0" : ""),
               date->month,
               (date->dayOfMonth < 10 ? "0" : ""),
               date->dayOfMonth);
#endif //EMBER_AF_PRINT_ENABLE
}

void emberAfPrintDateln(const EmberAfDate * date)
{
  emberAfPrintDate(date);
  emberAfPrintln(emberAfPrintActiveArea, "");
}

// *******************************************************
// emberAfPrintTime and emberAfSetTime are convienience methods for setting
// and displaying human readable times.

// Expects to be passed a ZigBee time which is the number of seconds
// since the year 2000
void emberAfSetTime(int32u utcTime) {
#ifdef EMBER_AF_PLUGIN_TIME_SERVER
  emAfTimeClusterServerSetCurrentTime(utcTime);
#endif //EMBER_AF_PLUGIN_TIME_SERVER
  emberAfSetTimeCallback(utcTime);
}

int32u emberAfGetCurrentTime(void)
{
#ifdef EMBER_AF_PLUGIN_TIME_SERVER
  return emAfTimeClusterServerGetCurrentTime();
#else
  return emberAfGetCurrentTimeCallback();
#endif
}

