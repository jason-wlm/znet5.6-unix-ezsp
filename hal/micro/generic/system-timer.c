/**
 * File: hal/micro/generic/system-timer.c
 * Description: simulation files for the system timer part of the HAL
 *
 * Copyright 2008 by Ember Corporation. All rights reserved.
 */

#include <sys/time.h>

#include PLATFORM_HEADER

int32u halCommonGetInt32uMillisecondTick(void)
{
  struct timeval tv;
  int32u now;  

  gettimeofday(&tv, NULL);
  now = (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
  return now;
}

int16u halCommonGetInt16uMillisecondTick(void)
{
  return (int16u)halCommonGetInt32uMillisecondTick();
}

int16u halCommonGetInt16uQuarterSecondTick(void)
{
  return (int16u)(halCommonGetInt32uMillisecondTick() >> 8);
}
