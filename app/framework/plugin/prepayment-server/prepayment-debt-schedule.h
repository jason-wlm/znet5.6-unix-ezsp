#ifndef _PREPAYMENT_DEBT_SCHEDULE_H_
#define _PREPAYMENT_DEBT_SCHEDULE_H_


typedef struct{
  int32u issuerEventId;
  int16u collectionTime;
  int32u nextCollectionTimeUtc;
  int32u firstCollectionTimeSec; // Time of first collection
  int8u  collectionFrequency;   // Time between collections (hour, day, week, month, etc)
  int8u  debtType;
  int8u  endpoint;
} emDebtScheduleEntry;


/**
 * @brief Initializes the debt schedule.
 *
 **/
void emberAfPluginPrepaymentServerInitDebtSchedule( void );


/**
 * @brief Schedules a received debt repayment event.
 * @param endpoint The endpoint number of the prepayment server.
 * @param issuerEventId The issuerEventId sent in the received Change Debt command.
 * @param debtType Indicates if the debt applies to Debt #1, #2, or #3 attributes.
 * @param collectionTime The time offset (in minutes) relative to midnight when debt collection should take place.
 * @param startTime The UTC time that denotes the time at which the debt collection should start, subject to the collectionTime.
 * @param collectionFrequency Specifies the period over which each collection should take place (hourly, daily, weekly, etc).
 *
 **/
void emberAfPluginPrepaymentServerScheduleDebtRepayment( int8u endpoint, int32u issuerEventId, int8u debtType, int16u collectionTime, int32u startTime, int8u collectionFrequency );


/**
 * @brief Checks all debt schedules to see if any debt collections must be performed.
 * @param endpoint The endpoint number of the prepayment server.
 * @param timeNowUtc Specifies the current UTC time.
 *
 **/
void emberAfPrepaymentServerSetDebtMode( int8u endpoint, int32u timeNowUtc );


/**
 * @brief Determines the number of seconds until the next debt collection event will occur.
 * @param timeNowUtc Specifies the current UTC time.
 *
 **/
int32u emberAfPrepaymentServerSecondsUntilDebtCollectionEvent( int32u timeNowUtc );





#endif  // #ifndef _PREPAYMENT_DEBT_SCHEDULE_H_


