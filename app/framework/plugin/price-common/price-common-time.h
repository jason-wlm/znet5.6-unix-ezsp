#ifndef _PRICE_COMMON_TIME_H_
#define _PRICE_COMMON_TIME_H_

enum{
  START_OF_TIMEBASE = 0x00,
  END_OF_TIMEBASE = 0x01
};

enum{
  DURATION_TYPE_MINS = 0x00,
  DURATION_TYPE_DAYS_START   = 0x01,
  DURATION_TYPE_DAYS_END     = 0x11,
  DURATION_TYPE_WEEKS_START  = 0x02,
  DURATION_TYPE_WEEKS_END    = 0x12,
  DURATION_TYPE_MONTHS_START = 0x03,
  DURATION_TYPE_MONTHS_END   = 0x13,
};

/**
 * @brief Converts the duration to a number of seconds based on the duration type parameter.
 *
 */
int32u emberAfPluginPriceCommonClusterConvertDurationToSeconds( int32u startTimeUtc, int32u duration, int8u durationType );

/**
 * @brief Calculates a new UTC start time value based on the duration type parameter.
 *
 */
int32u emberAfPluginPriceCommonClusterGetAdjustedStartTime(int32u startTimeUtc, int8u durationType);

#endif // #ifndef _PRICE_COMMON_TIME_H_
