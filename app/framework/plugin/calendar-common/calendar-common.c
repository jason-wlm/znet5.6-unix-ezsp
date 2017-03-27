// ****************************************************************************
// * calendar-common.c
// *
// *
// * Copyright 2014 Silicon Laboratories, Inc.                              *80*
// ****************************************************************************

#include "app/framework/include/af.h"
#include "calendar-common.h"

//-----------------------------------------------------------------------------
// Globals

EmberAfCalendarStruct calendars[EMBER_AF_PLUGIN_CALENDAR_COMMON_TOTAL_CALENDARS];

/*
 * From GBCS V0.8.1 section 10.4.2.11
 *
 * When processing a command where Issuer Calendar ID has the value 0xFFFFFFFF
 * or 0xFFFFFFFE, a GPF or GSME shall interpret 0xFFFFFFFF as meaning the
 * currently in force Tariff Switching Table calendar and 0xFFFFFFFE as meaning
 * the currently in force Non-Disablement Calendar
 */
#if defined(EMBER_AF_PLUGIN_GAS_PROXY_FUNCTION)
int32u tariffSwitchingCalendarId = EMBER_AF_PLUGIN_CALENDAR_COMMON_INVALID_CALENDAR_ID;
int32u nonDisablementCalendarId  = EMBER_AF_PLUGIN_CALENDAR_COMMON_INVALID_CALENDAR_ID;
#endif

//-----------------------------------------------------------------------------

void emberAfPluginCalendarCommonInitCallback(int8u endpoint)
{
  int8u i;
  for (i = 0; i < EMBER_AF_PLUGIN_CALENDAR_COMMON_TOTAL_CALENDARS; i++) {
    MEMSET(&(calendars[i]), 0, sizeof(EmberAfCalendarStruct));
    calendars[i].calendarId = EMBER_AF_PLUGIN_CALENDAR_COMMON_INVALID_CALENDAR_ID;
  }
}

int8u emberAfPluginCalendarCommonGetCalendarById(int32u calendarId,
                                                 int32u providerId)
{
  int8u i;

#if defined(EMBER_AF_PLUGIN_GAS_PROXY_FUNCTION)
  if (calendarId == GBCS_TARIFF_SWITCHING_CALENDAR_ID) {
    calendarId = tariffSwitchingCalendarId;
  } else if (calendarId == GBCS_NON_DISABLEMENT_CALENDAR_ID) {
    calendarId = nonDisablementCalendarId;
  }
#endif

  for (i = 0; i < EMBER_AF_PLUGIN_CALENDAR_COMMON_TOTAL_CALENDARS; i++) {
    if (calendarId == calendars[i].calendarId
        && (providerId == EMBER_AF_PLUGIN_CALENDAR_COMMON_WILDCARD_PROVIDER_ID
            || providerId == calendars[i].providerId)) {
      return i;
    }
  }
  return EMBER_AF_PLUGIN_CALENDAR_COMMON_INVALID_INDEX;
}

EmberAfCalendarStruct * findCalendarByCalId(int32u issuerCalendarId){
  int8u i;
  for (i=0; i<EMBER_AF_PLUGIN_CALENDAR_COMMON_TOTAL_CALENDARS; i++){
    EmberAfCalendarStruct * cal = &calendars[i];
    if (cal->calendarId == issuerCalendarId){
      return cal;
    }
  }
  emberAfCalendarClusterPrintln("ERR: Unable to find calendar with id(0x%4X) ", issuerCalendarId);
  return NULL;
}


int32u emberAfPluginCalendarCommonEndTimeUtc(const EmberAfCalendarStruct *calendar)
{
  int32u endTimeUtc = MAX_INT32U_VALUE;
  int8u i;
  for (i = 0; i < EMBER_AF_PLUGIN_CALENDAR_COMMON_TOTAL_CALENDARS; i++) {
    if (calendar->providerId == calendars[i].providerId
        && calendar->issuerEventId < calendars[i].issuerEventId
        && calendar->startTimeUtc < calendars[i].startTimeUtc
        && calendar->calendarType == calendars[i].calendarType
        && calendars[i].startTimeUtc < endTimeUtc) {
      endTimeUtc = calendars[i].startTimeUtc;
    }
  }

  return endTimeUtc;
}

boolean emberAfCalendarCommonSetCalInfo(int8u index,
                                        int32u providerId,
                                        int32u issuerEventId,
                                        int32u issuerCalendarId,
                                        int32u startTimeUtc,
                                        int8u calendarType,
                                        int8u *calendarName,
                                        int8u numberOfSeasons,
                                        int8u numberOfWeekProfiles,
                                        int8u numberOfDayProfiles)
{
  EmberAfCalendarStruct * cal;
  if (index >= EMBER_AF_PLUGIN_CALENDAR_COMMON_TOTAL_CALENDARS) {
    emberAfCalendarClusterPrintln("Index must be in the range of 0 to %d", EMBER_AF_PLUGIN_CALENDAR_COMMON_TOTAL_CALENDARS - 1);
    return FALSE;
  }

  if (calendarName == NULL || calendarName[0] > EMBER_AF_PLUGIN_CALENDAR_MAX_CALENDAR_NAME_LENGTH){
    emberAfCalendarClusterPrintln("Invalid calendar name!");
    return FALSE;
  }

  cal = &calendars[index];
  cal->providerId = providerId;
  cal->issuerEventId = issuerEventId;
  cal->calendarId = issuerCalendarId;
  cal->startTimeUtc = startTimeUtc;
  cal->calendarType = calendarType;
  MEMCOPY(cal->name, calendarName, calendarName[0] + 1);
  cal->numberOfSeasons = numberOfSeasons;
  cal->numberOfWeekProfiles = numberOfWeekProfiles;
  cal->numberOfDayProfiles = numberOfDayProfiles;
  cal->flags = 0;
  return TRUE;
}







boolean emberAfCalendarCommonAddCalInfo(int32u providerId,
                                        int32u issuerEventId,
                                        int32u issuerCalendarId,
                                        int32u startTimeUtc,
                                        int8u calendarType,
                                        int8u *calendarName,
                                        int8u numberOfSeasons,
                                        int8u numberOfWeekProfiles,
                                        int8u numberOfDayProfiles)
{
  boolean status = FALSE;
  int8u i = 0;
  int8u matchingEntryIndex = 0xFF;
  int8u smallestEventIdEntryIndex = 0x0;
  EmberAfCalendarStruct * cal;

  // try to find an existing entry to overwrite.
  for (i=0; i<EMBER_AF_PLUGIN_CALENDAR_COMMON_TOTAL_CALENDARS; i++) {
    EmberAfCalendarStruct * cal = &calendars[i];
    if (cal->providerId == providerId
        && cal->calendarId == issuerCalendarId){
      matchingEntryIndex = i;
    }
    
    if (cal->issuerEventId < calendars[smallestEventIdEntryIndex].issuerEventId){
      smallestEventIdEntryIndex = i;
    }
  }

  if (matchingEntryIndex == 0xFF){
    i = smallestEventIdEntryIndex;
  } else {
    i = matchingEntryIndex;
  }

  cal = &calendars[i];

  if (numberOfSeasons > EMBER_AF_PLUGIN_CALENDAR_COMMON_SEASON_PROFILE_MAX){
    emberAfCalendarClusterPrintln("ERR: Insufficient space for requested number of seasons(%d)!", numberOfSeasons);
    status = FALSE;
    goto kickout;
  }
  if (numberOfWeekProfiles > EMBER_AF_PLUGIN_CALENDAR_COMMON_WEEK_PROFILE_MAX){
    emberAfCalendarClusterPrintln("ERR: Insufficient space for requested number of week profiles(%d)!", numberOfWeekProfiles);
    status = FALSE;
    goto kickout;
  }
  if (numberOfDayProfiles > EMBER_AF_PLUGIN_CALENDAR_COMMON_DAY_PROFILE_MAX){
    emberAfCalendarClusterPrintln("ERR: Insufficient space for requested number of day profiles(%d)!", numberOfDayProfiles);
    status = FALSE;
    goto kickout;
  }

  // calendars must be replaced as a whole.
  MEMSET(cal, 0x00, sizeof(EmberAfCalendarStruct));
  cal->providerId = providerId;
  cal->issuerEventId = issuerEventId;
  cal->calendarId = issuerCalendarId;
  cal->startTimeUtc = startTimeUtc;
  cal->calendarType = calendarType;
  MEMCOPY(cal->name, calendarName, calendarName[0] + 1);
  cal->numberOfSeasons = numberOfSeasons;
  cal->numberOfWeekProfiles = numberOfWeekProfiles;
  cal->numberOfDayProfiles = numberOfDayProfiles;
  cal->flags = 0;
  status = TRUE; 

#if defined(EMBER_AF_PLUGIN_GAS_PROXY_FUNCTION)
  if (cal->calendarType == EMBER_ZCL_CALENDAR_TYPE_DELIVERED_CALENDAR) {
    tariffSwitchingCalendarId = cal->calendarId;
  } else if (cal->calendarType == EMBER_ZCL_CALENDAR_TYPE_FRIENDLY_CREDIT_CALENDAR) {
    nonDisablementCalendarId = cal->calendarId;
  }
#endif

kickout:
  return status;
}

boolean emberAfCalendarCommonAddSpecialDaysInfo(int32u issuerCalendarId,
                                                int8u totalNumberOfSpecialDays,
                                                int8u * specialDaysEntries,
                                                int16u specialDaysEntriesLength)
{
  EmberAfCalendarStruct * cal = findCalendarByCalId(issuerCalendarId);
  boolean status = FALSE;
  int8u numberOfSpecialDaysEntries = specialDaysEntriesLength / SPECIAL_DAY_ENTRY_SIZE;
  int8u scheduleEntryIndex = 0;
  int16u minutesFromMidnight;
  int8u data;
  int8u i;

  if (cal == NULL){
    status = FALSE;
    goto kickout;
  }

  if (totalNumberOfSpecialDays > EMBER_AF_PLUGIN_CALENDAR_COMMON_SPECIAL_DAY_PROFILE_MAX){
    emberAfCalendarClusterPrintln("ERR: %d number of special days being added is above the current maximum(%d).",
                                  totalNumberOfSpecialDays,
                                  EMBER_AF_PLUGIN_CALENDAR_COMMON_SPECIAL_DAY_PROFILE_MAX);
    status = FALSE;
    goto kickout;
  }

  if (totalNumberOfSpecialDays != numberOfSpecialDaysEntries){
    emberAfCalendarClusterPrintln("ERR: adding special days with inconsistent information.");
    status = FALSE;
    goto kickout;
  }

  if ((specialDaysEntries == NULL) || (numberOfSpecialDaysEntries == 0)) {
    status = FALSE; 
    emberAfCalendarClusterPrintln("ERR: Unable to add special days.");
    goto kickout;
  }
    
  for (i=0; i<numberOfSpecialDaysEntries; i++){
    EmberAfDate startDate = {0};
    int8u normalDayId;
    int8u index;

    emberAfGetDate(specialDaysEntries,
                   SPECIAL_DAY_ENTRY_SIZE * i,
                   specialDaysEntriesLength,
                   &startDate);
    normalDayId = emberAfGetInt8u(specialDaysEntries,  
                                  SPECIAL_DAY_ENTRY_SIZE * i + sizeof(EmberAfDate),
                                  specialDaysEntriesLength);
    if (cal->numberOfSpecialDayProfiles < EMBER_AF_PLUGIN_CALENDAR_COMMON_SPECIAL_DAY_PROFILE_MAX){
      int8u normalDayIndex;
      boolean update = TRUE;

      // find corresponding normal day index
      for (normalDayIndex=0; normalDayIndex<EMBER_AF_PLUGIN_CALENDAR_COMMON_DAY_PROFILE_MAX; normalDayIndex++){
        if (cal->normalDays[normalDayIndex].id == normalDayId){
          int8u index;
          // check for redundant specialDays
          for (index=0; index<cal->numberOfSpecialDayProfiles; index++){
            EmberAfCalendarSpecialDayStruct * specialDay = &cal->specialDays[index];
            if ((specialDay->normalDayIndex == normalDayIndex)
                && (emberAfCompareDates(&specialDay->startDate, &startDate) == 0)){
              update = FALSE;
            }
          }

          break;
        }
      }
 
      if(update){
        cal->specialDays[cal->numberOfSpecialDayProfiles].normalDayIndex = normalDayIndex;
        cal->specialDays[cal->numberOfSpecialDayProfiles].flags = 0;
        cal->specialDays[cal->numberOfSpecialDayProfiles].startDate = startDate;

        emberAfCalendarClusterPrintln("Updated: Calendar(calId=0x%4X)", 
                                      cal->calendarId);
        emberAfCalendarClusterPrintln("         SpecialDays[%d]", cal->numberOfSpecialDayProfiles);
        emberAfCalendarClusterPrint  ("           startDate: ");
        emberAfPrintDateln(&cal->specialDays[cal->numberOfSpecialDayProfiles].startDate);
        emberAfCalendarClusterPrintln( "           normalDayIndex: %d", cal->specialDays[cal->numberOfSpecialDayProfiles].normalDayIndex);
        cal->numberOfSpecialDayProfiles++;
      } else {
        emberAfCalendarClusterPrintln("ERR: Invalid dayId! Unable to store SpecialDays info.");
      }
    } else {
      emberAfCalendarClusterPrintln("ERR: Insufficient space to store more SpecialDays info!");
    }
  }
  status = TRUE;

kickout:
  return status;
}


boolean emberAfCalendarCommonAddDayProfInfo(int32u issuerCalendarId,
                                            int8u dayId,
                                            int8u * dayScheduleEntries,
                                            int16u dayScheduleEntriesLength)
{
  EmberAfCalendarStruct * cal = findCalendarByCalId(issuerCalendarId);
  boolean status = FALSE;
  int8u numberOfScheduledEntries = dayScheduleEntriesLength / SCHEDULE_ENTRY_SIZE;
  int8u scheduleEntryIndex = 0;

  if (cal == NULL){
    emberAfCalendarClusterPrintln("ERR: Unable to find calendar with id(0x%4X)",
                                  issuerCalendarId);
    status = FALSE;
    goto kickout;
  }

  if ((dayScheduleEntries == NULL)
      || (dayScheduleEntriesLength == 0)
      || ((dayScheduleEntriesLength % SCHEDULE_ENTRY_SIZE) != 0) // each struct occupy 3 bytes.
      || (cal->numberOfReceivedDayProfiles >= EMBER_AF_PLUGIN_CALENDAR_COMMON_DAY_PROFILE_MAX)
      || (numberOfScheduledEntries > EMBER_AF_PLUGIN_CALENDAR_COMMON_SCHEDULE_ENTRIES_MAX))
  {
    status = FALSE; 
    emberAfCalendarClusterPrintln("ERR: Unable to add DayProfile");
    goto kickout;
  }

  emberAfCalendarClusterPrintln("Updated: DayProfile[%d]", 
                                cal->numberOfReceivedDayProfiles);
  emberAfCalendarClusterPrintln("           DayId=%d", 
                                dayId);

  cal->normalDays[cal->numberOfReceivedDayProfiles].id = dayId;
  cal->normalDays[cal->numberOfReceivedDayProfiles].numberOfScheduleEntries = numberOfScheduledEntries;

  for (scheduleEntryIndex=0; scheduleEntryIndex<numberOfScheduledEntries; scheduleEntryIndex++){
    int8u priceTier;
    int16u minutesFromMidnight;
    EmberAfCalendarDayScheduleEntryStruct * normalDay;
    normalDay = &cal->normalDays[cal->numberOfReceivedDayProfiles].scheduleEntries[scheduleEntryIndex];
    minutesFromMidnight = emberAfGetInt16u(dayScheduleEntries,  
                                           scheduleEntryIndex*SCHEDULE_ENTRY_SIZE, 
                                           dayScheduleEntriesLength);
    priceTier = emberAfGetInt8u(dayScheduleEntries,  
                           scheduleEntryIndex*SCHEDULE_ENTRY_SIZE + 2, 
                           dayScheduleEntriesLength);
    normalDay->minutesFromMidnight =  minutesFromMidnight;
    normalDay->data =  priceTier;
    emberAfCalendarClusterPrintln("           ScheduledEntries[%d]", 
                                  scheduleEntryIndex);
    emberAfCalendarClusterPrintln("             startTime: 0x%2X from midnight", 
                                   minutesFromMidnight);
    emberAfCalendarClusterPrintln("             price tier: 0x%X", 
                                   priceTier);
  }

  cal->numberOfReceivedDayProfiles++;
  status = TRUE;

kickout:
  return status;
}

boolean emberAfCalendarCommonSetDayProfInfo(int8u index,
                                            int8u dayId,
                                            int8u entryId,
                                            int16u minutesFromMidnight,
                                            int8u data)
{
  if (index >= EMBER_AF_PLUGIN_CALENDAR_COMMON_TOTAL_CALENDARS) {
    emberAfCalendarClusterPrintln("Index must be in the range of 0 to %d", EMBER_AF_PLUGIN_CALENDAR_COMMON_TOTAL_CALENDARS - 1);
    return FALSE;
  } else if(dayId >= EMBER_AF_PLUGIN_CALENDAR_COMMON_DAY_PROFILE_MAX){
    emberAfCalendarClusterPrintln("DayId must be in the range of 0 to %d", EMBER_AF_PLUGIN_CALENDAR_COMMON_DAY_PROFILE_MAX - 1);
    return FALSE;
  } else if (entryId >= EMBER_AF_PLUGIN_CALENDAR_COMMON_SCHEDULE_ENTRIES_MAX){
    emberAfCalendarClusterPrintln("EntryId must be in the range of 0 to %d", EMBER_AF_PLUGIN_CALENDAR_COMMON_SCHEDULE_ENTRIES_MAX - 1);
    return FALSE;
  }

  calendars[index].normalDays[dayId].scheduleEntries[entryId].minutesFromMidnight =  minutesFromMidnight;
  calendars[index].normalDays[dayId].scheduleEntries[entryId].data = data;
  return TRUE;
}

boolean emberAfCalendarServerAddWeekProfInfo(int32u issuerCalendarId,
                                             int8u weekId,                            
                                             int8u dayIdRefMon,
                                             int8u dayIdRefTue,
                                             int8u dayIdRefWed,
                                             int8u dayIdRefThu,
                                             int8u dayIdRefFri,
                                             int8u dayIdRefSat,
                                             int8u dayIdRefSun)
{
  EmberAfCalendarStruct * cal = findCalendarByCalId(issuerCalendarId);
  int8u dayIdRefs[7];
  int8u dayCount = 7;
  int8u * normalDayIndexes;
  EmberAfCalendarWeekStruct * weeks;
  EmberAfCalendarDayStruct * normalDays;
  int8u dayIdRefsIndex;

  if ((cal == NULL)
      || (cal->numberOfReceivedWeekProfiles >= EMBER_AF_PLUGIN_CALENDAR_COMMON_WEEK_PROFILE_MAX)) {
    return FALSE;
  }

  dayIdRefs[0] = dayIdRefMon; 
  dayIdRefs[1] = dayIdRefTue; 
  dayIdRefs[2] = dayIdRefWed; 
  dayIdRefs[3] = dayIdRefThu; 
  dayIdRefs[4] = dayIdRefFri; 
  dayIdRefs[5] = dayIdRefSat; 
  dayIdRefs[6] = dayIdRefSun; 
  normalDays = (EmberAfCalendarDayStruct *)&cal->normalDays;
  normalDayIndexes = (int8u *)&cal->weeks[cal->numberOfReceivedWeekProfiles].normalDayIndexes;
  weeks = &cal->weeks[cal->numberOfReceivedWeekProfiles];

  cal->weeks[cal->numberOfReceivedWeekProfiles].id = weekId;
  emberAfCalendarClusterPrintln("Updated: WeekProfile[%d]",
                                cal->numberOfReceivedWeekProfiles);
  emberAfCalendarClusterPrintln("           weekId=%d", weekId);

  for (dayIdRefsIndex=0; dayIdRefsIndex<dayCount; dayIdRefsIndex++){
    int8u normalDayIndex;
    int8u dayId = dayIdRefs[dayIdRefsIndex];
    for (normalDayIndex=0; normalDayIndex<EMBER_AF_PLUGIN_CALENDAR_COMMON_DAY_PROFILE_MAX; normalDayIndex++){
      if (normalDays[normalDayIndex].id == dayId) {
        normalDayIndexes[dayIdRefsIndex] = normalDayIndex;
        emberAfCalendarClusterPrintln("           normalDayIndexes[%d]=%d",
                                      dayIdRefsIndex,
                                      normalDayIndex);

      }
    }
  }
  cal->numberOfReceivedWeekProfiles++;
  return TRUE;
}


boolean emberAfCalendarServerSetWeekProfInfo(int8u index,
                                             int8u weekId,
                                             int8u dayIdRefMon,
                                             int8u dayIdRefTue,
                                             int8u dayIdRefWed,
                                             int8u dayIdRefThu,
                                             int8u dayIdRefFri,
                                             int8u dayIdRefSat,
                                             int8u dayIdRefSun)
{
  if (index >= EMBER_AF_PLUGIN_CALENDAR_COMMON_TOTAL_CALENDARS) {
    emberAfCalendarClusterPrintln("Index must be in the range of 0 to %d", EMBER_AF_PLUGIN_CALENDAR_COMMON_TOTAL_CALENDARS - 1);
    return FALSE;
  } else if(weekId >= EMBER_AF_PLUGIN_CALENDAR_COMMON_WEEK_PROFILE_MAX){
    emberAfCalendarClusterPrintln("WeekId must be in the range of 0 to %d", EMBER_AF_PLUGIN_CALENDAR_COMMON_WEEK_PROFILE_MAX - 1);
    return FALSE;
  }

  calendars[index].weeks[weekId].normalDayIndexes[0] = dayIdRefMon; 
  calendars[index].weeks[weekId].normalDayIndexes[1] = dayIdRefTue; 
  calendars[index].weeks[weekId].normalDayIndexes[2] = dayIdRefWed; 
  calendars[index].weeks[weekId].normalDayIndexes[3] = dayIdRefThu; 
  calendars[index].weeks[weekId].normalDayIndexes[4] = dayIdRefFri; 
  calendars[index].weeks[weekId].normalDayIndexes[5] = dayIdRefSat; 
  calendars[index].weeks[weekId].normalDayIndexes[6] = dayIdRefSun; 
  return TRUE;
}

boolean emberAfCalendarServerAddSeasonsInfo(int32u issuerCalendarId,
                                            int8u * seasonsEntries, 
                                            int8u seasonsEntriesLength)
{
  boolean status = FALSE;
  EmberAfCalendarStruct * cal = findCalendarByCalId(issuerCalendarId);
  int8u seasonEntryCount = seasonsEntriesLength/SEASON_ENTRY_SIZE;
  int8u seasonEntryIndex;
  if (cal == NULL){
    status = FALSE;
    goto kickout;
  }

  if (cal->numberOfReceivedSeasons + seasonEntryCount > cal->numberOfSeasons){
    status = FALSE;
    goto kickout;
  }

  for (seasonEntryIndex=0; seasonEntryIndex<seasonEntryCount; seasonEntryIndex++){
    EmberAfCalendarSeasonStruct  * season = &cal->seasons[cal->numberOfReceivedSeasons];
    EmberAfDate startDate;
    int8u weekId;
    emberAfGetDate(seasonsEntries,
                   seasonEntryIndex*SEASON_ENTRY_SIZE,
                   seasonsEntriesLength, 
                   &startDate);
    weekId = emberAfGetInt8u(seasonsEntries,
                                seasonEntryIndex*SEASON_ENTRY_SIZE + sizeof(EmberAfDate),
                                seasonsEntriesLength);
    season->startDate = startDate;
    emberAfCalendarClusterPrintln( "Updated: Seasons[%d]", cal->numberOfReceivedSeasons);
    emberAfCalendarClusterPrint("            startDate: ");
    emberAfPrintDateln(&startDate);
    {
      // search for week index.
      int8u i;
      for (i=0; i<EMBER_AF_PLUGIN_CALENDAR_COMMON_WEEK_PROFILE_MAX; i++){
        if (cal->weeks[i].id == weekId){
          season->weekIndex = i;
        }
      }
    }

    emberAfCalendarClusterPrintln("            weekIndex: %d", season->weekIndex);

    cal->numberOfReceivedSeasons++;
  }
  status = TRUE;

kickout:
  return status;
}

boolean emberAfCalendarServerSetSeasonsInfo(int8u index,
                                            int8u seasonId,
                                            EmberAfDate startDate,
                                            int8u weekIdRef)
{
  if (index >= EMBER_AF_PLUGIN_CALENDAR_COMMON_TOTAL_CALENDARS) {
    emberAfCalendarClusterPrintln("Index must be in the range of 0 to %d", EMBER_AF_PLUGIN_CALENDAR_COMMON_TOTAL_CALENDARS - 1);
    return FALSE;
  } else if(seasonId >= EMBER_AF_PLUGIN_CALENDAR_COMMON_SEASON_PROFILE_MAX){
    emberAfCalendarClusterPrintln("SeasonId must be in the range of 0 to %d", EMBER_AF_PLUGIN_CALENDAR_COMMON_SEASON_PROFILE_MAX - 1);
    return FALSE;
  }

  calendars[index].seasons[seasonId].startDate = startDate;
  calendars[index].seasons[seasonId].weekIndex = weekIdRef;
  return TRUE;
}
