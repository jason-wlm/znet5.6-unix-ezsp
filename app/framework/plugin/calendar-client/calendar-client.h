// Copyright 2014 Silicon Laboratories, Inc.


#define EMBER_AF_PLUGIN_CALENDAR_CLIENT_INVALID_CALENDAR_ID 0xFFFFFFFF
#define EMBER_AF_CALENDAR_MAXIMUM_CALENDAR_NAME_LENGTH 12

typedef struct {
  EmberAfDate seasonStartDate;
  int8u weekIdRef;
} EmberAfCalendarSeason;

typedef struct {
  boolean inUse;
  int8u dayIdRefMonday;
  int8u dayIdRefTuesday;
  int8u dayIdRefWednesday;
  int8u dayIdRefThursday;
  int8u dayIdRefFriday;
  int8u dayIdRefSaturday;
  int8u dayIdRefSunday;
} EmberAfCalendarWeekProfile;

// All valid calendar types have the same general format for schedule entries:
// a two-byte start time followed by a one-byte, type-specific value.  The code
// in this plugin takes advantage of this similarity to simply the logic.  If
// new types are added, the code will need to change.  See
// emberAfCalendarClusterPublishDayProfileCallback.
typedef union {
  struct {
    int16u startTimeM;
    int8u priceTier;
  } rateSwitchTime;
  struct {
    int16u startTimeM;
    boolean friendlyCreditEnable;
  } friendlyCreditSwitchTime;
  struct {
    int16u startTimeM;
    int8u auxiliaryLoadSwitchState;
  } auxilliaryLoadSwitchTime;
} EmberAfCalendarScheduleEntry;

typedef struct {
  boolean inUse;
  int8u numberOfScheduleEntries;
  int8u receivedScheduleEntries;
  EmberAfCalendarScheduleEntry scheduleEntries[EMBER_AF_PLUGIN_CALENDAR_CLIENT_SCHEDULE_ENTRIES];
} EmberAfCalendarDayProfile;

typedef struct {
  EmberAfDate specialDayDate;
  int8u dayIdRef;
} EmberAfCalendarSpecialDayEntry;

typedef struct {
  boolean inUse;
  int32u startTimeUtc;
  int8u numberOfSpecialDayEntries;
  int8u receivedSpecialDayEntries;
  EmberAfCalendarSpecialDayEntry specialDayEntries[EMBER_AF_PLUGIN_CALENDAR_CLIENT_SPECIAL_DAY_ENTRIES];
} EmberAfCalendarSpecialDayProfile;

typedef struct {
  boolean inUse;
  int32u providerId;
  int32u issuerEventId;
  int32u issuerCalendarId;
  int32u startTimeUtc;
  EmberAfCalendarType calendarType;
  int8u calendarName[EMBER_AF_CALENDAR_MAXIMUM_CALENDAR_NAME_LENGTH + 1];
  int8u numberOfSeasons;
  int8u receivedSeasons;
  int8u numberOfWeekProfiles;
  int8u numberOfDayProfiles;
  EmberAfCalendarSeason seasons[EMBER_AF_PLUGIN_CALENDAR_CLIENT_SEASONS];
  EmberAfCalendarWeekProfile weekProfiles[EMBER_AF_PLUGIN_CALENDAR_CLIENT_WEEK_PROFILES];
  EmberAfCalendarDayProfile dayProfiles[EMBER_AF_PLUGIN_CALENDAR_CLIENT_DAY_PROFILES];
  EmberAfCalendarSpecialDayProfile specialDayProfile;
} EmberAfCalendar;


/**
 * @brief Gets the first calendar index based on the calendar type.
 *
 * @param endpoint The relevant endpoint.
 * @param calendarType The type of calendar that should be searched for in the table.
 * @return The index of the first matching calendar, or EMBER_AF_PLUGIN_CALENDAR_CLIENT_CALENDARS
 * if a match cannot be found.
 *
 **/
int8u emberAfPluginCalendarClientGetCalendarIndexByType( int8u endpoint, int8u calendarType );


/**
 * @brief Gets the calendar ID at the specified index.
 *
 * @param endpoint The relevant endpoint.
 * @param index The index in the calendar table whose calendar ID should be returned.
 * @return The calendar ID of the calendar at the specified index.  If index is out of bounds, EMBER_AF_PLUGIN_CALENDAR_CLIENT_INVALID_CALENDAR_ID will be returned.
 * if a match cannot be found.
 *
 **/
int32u emberAfPluginCalendarClientGetCalendarId( int8u endpoint, int8u index );


