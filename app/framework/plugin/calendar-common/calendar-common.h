#define EMBER_AF_PLUGIN_CALENDAR_COMMON_INVALID_SCHEDULE_ENTRY 0xFFFF
#define EMBER_AF_PLUGIN_CALENDAR_COMMON_INVALID_ID 0xFF
#define EMBER_AF_PLUGIN_CALENDAR_MAX_CALENDAR_NAME_LENGTH 12
#define EMBER_AF_PLUGIN_CALENDAR_COMMON_INVALID_INDEX 0xFF

#define SCHEDULE_ENTRY_SIZE (3)
typedef struct {
  int16u minutesFromMidnight;
  
  // the format of the actual data in the entry depends on the calendar type
  //   for calendar type 00 - 0x02, it is a rate switch time
  //     the data is a price tier enum (8-bit)
  //   for calendar type 0x03 it is friendly credit switch time
  //     the data is a boolean (8-bit) meaning friendly credit enabled
  //   for calendar type 0x04 it is an auxilliary load switch time
  //     the data is a bitmap (8-bit)
  int8u data;  
} EmberAfCalendarDayScheduleEntryStruct ;

// Season start date (4-bytes) and week ID ref (1-byte)
#define SEASON_ENTRY_SIZE (5)

typedef struct {
  EmberAfCalendarDayScheduleEntryStruct scheduleEntries[EMBER_AF_PLUGIN_CALENDAR_COMMON_SCHEDULE_ENTRIES_MAX];
  int8u id;
  int8u numberOfScheduleEntries;
} EmberAfCalendarDayStruct;

// Special day date (4 bytes) and Day ID ref (1-byte)
#define SPECIAL_DAY_ENTRY_SIZE (5)
typedef struct {
  EmberAfDate startDate;
  int8u normalDayIndex;
  int8u flags;
} EmberAfCalendarSpecialDayStruct;

#define EMBER_AF_PLUGIN_CALENDAR_COMMON_MONDAY_INDEX (0)
#define EMBER_AF_PLUGIN_CALENDAR_COMMON_SUNDAY_INDEX (6)
#define EMBER_AF_DAYS_IN_THE_WEEK (7)

typedef struct {
  int8u normalDayIndexes[EMBER_AF_DAYS_IN_THE_WEEK];
  int8u id;
} EmberAfCalendarWeekStruct;

typedef struct {
  EmberAfDate startDate;
  int8u weekIndex;
} EmberAfCalendarSeasonStruct;

#define EMBER_AF_PLUGIN_CALENDAR_COMMON_INVALID_CALENDAR_ID 0xFFFFFFFF
#define EMBER_AF_PLUGIN_CALENDAR_COMMON_WILDCARD_CALENDAR_ID 0x00000000
#define EMBER_AF_PLUGIN_CALENDAR_COMMON_WILDCARD_PROVIDER_ID 0xFFFFFFFF
#define EMBER_AF_PLUGIN_CALENDAR_COMMON_WILDCARD_ISSUER_ID 0xFFFFFFFF
#define EMBER_AF_PLUGIN_CALENDAR_COMMON_WILDCARD_CALENDAR_TYPE 0xFF

enum {
  EMBER_AF_PLUGIN_CALENDAR_COMMON_FLAGS_SENT = 0x01,
};

typedef struct {
  EmberAfCalendarWeekStruct weeks[EMBER_AF_PLUGIN_CALENDAR_COMMON_WEEK_PROFILE_MAX];
  EmberAfCalendarDayStruct normalDays[EMBER_AF_PLUGIN_CALENDAR_COMMON_DAY_PROFILE_MAX];
  EmberAfCalendarSpecialDayStruct specialDays[EMBER_AF_PLUGIN_CALENDAR_COMMON_SPECIAL_DAY_PROFILE_MAX];
  EmberAfCalendarSeasonStruct seasons[EMBER_AF_PLUGIN_CALENDAR_COMMON_SEASON_PROFILE_MAX];
  int32u providerId;
  int32u issuerEventId;
  int32u calendarId;
  int32u startTimeUtc;
  int8u name[EMBER_AF_PLUGIN_CALENDAR_MAX_CALENDAR_NAME_LENGTH + 1];
  int8u calendarType;
  int8u numberOfSeasons;
  int8u numberOfWeekProfiles;
  int8u numberOfDayProfiles;
  int8u numberOfSpecialDayProfiles;

  /* these "received" counter don't really belong here. it's here to help with
   * replaying TOM messages correctly. They'll serve as destination index
   * for the next publish command.
   */
  int8u numberOfReceivedSeasons;
  int8u numberOfReceivedWeekProfiles;
  int8u numberOfReceivedDayProfiles;
  int8u flags;
} EmberAfCalendarStruct;

extern EmberAfCalendarStruct calendars[];
#if defined(EMBER_AF_PLUGIN_GAS_PROXY_FUNCTION)
#define GBCS_TARIFF_SWITCHING_CALENDAR_ID 0xFFFFFFFF
#define GBCS_NON_DISABLEMENT_CALENDAR_ID  0xFFFFFFFE
extern int32u tariffSwitchingCalendarId;
extern int32u nonDisablementCalendarId;
#endif

int8u emberAfPluginCalendarCommonGetCalendarById(int32u calendarId,
                                                 int32u providerId);
int32u emberAfPluginCalendarCommonEndTimeUtc(const EmberAfCalendarStruct *calendar);
boolean emberAfCalendarCommonSetCalInfo(int8u index,
                                        int32u providerId,
                                        int32u issuerEventId,
                                        int32u issuerCalendarId,
                                        int32u startTimeUtc,
                                        int8u calendarType,
                                        int8u *calendarName,
                                        int8u numberOfSeasons,
                                        int8u numberOfWeekProfiles,
                                        int8u numberOfDayProfiles);

/* @brief add new entry corresponding to PublishCalendar command.
 *
 * This function will try to handle the new entry in following method: 
 * 
 * 1) Try to apply new data to a matching existing entry. 
 *    Fields such as providerId, issuerEventId, and startTime, will be used.
 * 3) Overwrite the oldest entry (one with smallest event id) with new info.
 *
 */
boolean emberAfCalendarCommonAddCalInfo(int32u providerId,
                                        int32u issuerEventId,
                                        int32u issuerCalendarId,
                                        int32u startTimeUtc,
                                        int8u calendarType,
                                        int8u *calendarName,
                                        int8u numberOfSeasons,
                                        int8u numberOfWeekProfiles,
                                        int8u numberOfDayProfiles);
boolean emberAfCalendarServerSetSeasonsInfo(int8u index,
                                            int8u seasonId,
                                            EmberAfDate startDate,
                                            int8u weekIndex);
boolean emberAfCalendarServerAddSeasonsInfo(int32u issuerCalendarId,
                                            int8u * seasonsEntries, 
                                            int8u seasonsEntriesLength);

boolean emberAfCalendarCommonSetDayProfInfo(int8u index,
                                            int8u dayId,
                                            int8u entryId,
                                            int16u minutesFromMidnight,
                                            int8u data);
boolean emberAfCalendarCommonAddDayProfInfo(int32u issuerCalendarId,
                                            int8u dayId,
                                            int8u * dayScheduleEntries,
                                            int16u dayScheduleEntriesLength);
boolean emberAfCalendarServerSetWeekProfInfo(int8u index,
                                             int8u weekId,
                                             int8u dayIdRefMon,
                                             int8u dayIdRefTue,
                                             int8u dayIdRefWed,
                                             int8u dayIdRefThu,
                                             int8u dayIdRefFri,
                                             int8u dayIdRefSat,
                                             int8u dayIdRefSun);
boolean emberAfCalendarServerAddWeekProfInfo(int32u issuerCalendarId,
                                             int8u weekId,
                                             int8u dayIdRefMon,
                                             int8u dayIdRefTue,
                                             int8u dayIdRefWed,
                                             int8u dayIdRefThu,
                                             int8u dayIdRefFri,
                                             int8u dayIdRefSat,
                                             int8u dayIdRefSun);

/* @brief Updating special days info of the specified calendar.
 *
 * This function assumes that the value of totalNumberOfSpecialDays will match
 * up with the info passed in between specialDaysEntries/specialDaysEntriesLength.
 *
 */
boolean emberAfCalendarCommonAddSpecialDaysInfo(int32u issuerCalendarId,
                                                int8u totalNumberOfSpecialDays,
                                                int8u * specialDaysEntries,
                                                int16u specialDaysEntriesLength);
