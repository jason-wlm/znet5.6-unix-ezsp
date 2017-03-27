// *******************************************************************                                                                                        
// * calendar-server-cli.c
// *
// *
// * Copyright 2014 by Silicon Laboratories. All rights reserved.           *80*                                                                              
// ******************************************************************* 

#include "app/framework/include/af.h"
#include "app/framework/plugin/calendar-common/calendar-common.h"
#include "calendar-server.h"

#ifndef EMBER_AF_GENERATE_CLI
  #error The Calendar Server plugin is not compatible with the legacy CLI.
#endif

// plugin calendar-server publish-calendar <nodeId:2> <srcEndpoint:1> <dstEndpoint:1> <calendarIndex:1>
void emAfCalendarServerCliPublishCalendar(void)
{
  EmberNodeId nodeId = (EmberNodeId)emberUnsignedCommandArgument(0);
  int8u srcEndpoint = (int8u)emberUnsignedCommandArgument(1);
  int8u dstEndpoint = (int8u)emberUnsignedCommandArgument(2);
  int8u calendarIndex = (int8u)emberUnsignedCommandArgument(3);
  emberAfCalendarServerPublishCalendarMessage(nodeId,
                                              srcEndpoint,
                                              dstEndpoint,
                                              calendarIndex);
}

// plugin calendar-server publish-day-profiles <nodeId:2> <srcEndpoint:1> <dstEndpoint:1> <calendarIndex:1> <dayIndex:1>
void emAfCalendarServerCliPublishDayProfiles(void)
{
  EmberNodeId nodeId = (EmberNodeId)emberUnsignedCommandArgument(0);
  int8u srcEndpoint = (int8u)emberUnsignedCommandArgument(1);
  int8u dstEndpoint = (int8u)emberUnsignedCommandArgument(2);
  int8u calendarIndex = (int8u)emberUnsignedCommandArgument(3);
  int8u dayIndex = (int8u)emberUnsignedCommandArgument(4);
  emberAfCalendarServerPublishDayProfilesMessage(nodeId,
                                                 srcEndpoint,
                                                 dstEndpoint,
                                                 calendarIndex,
                                                 dayIndex);
}

// plugin calendar-server publish-week-profile <nodeId:2> <srcEndpoint:1> <dstEndpoint:1> <calendarIndex:1> <weekIndex:1>
void emAfCalendarServerCliPublishWeekProfile(void)
{
  EmberNodeId nodeId = (EmberNodeId)emberUnsignedCommandArgument(0);
  int8u srcEndpoint = (int8u)emberUnsignedCommandArgument(1);
  int8u dstEndpoint = (int8u)emberUnsignedCommandArgument(2);
  int8u calendarIndex = (int8u)emberUnsignedCommandArgument(3);
  int8u weekIndex = (int8u)emberUnsignedCommandArgument(4);
  emberAfCalendarServerPublishDayProfilesMessage(nodeId,
                                                 srcEndpoint,
                                                 dstEndpoint,
                                                 calendarIndex,
                                                 weekIndex);
}

// plugin calendar-server publish-week-profile <nodeId:2> <srcEndpoint:1> <dstEndpoint:1> <calendarIndex:1>
void emAfCalendarServerCliPublishSeasons(void)
{
  EmberNodeId nodeId = (EmberNodeId)emberUnsignedCommandArgument(0);
  int8u srcEndpoint = (int8u)emberUnsignedCommandArgument(1);
  int8u dstEndpoint = (int8u)emberUnsignedCommandArgument(2);
  int8u calendarIndex = (int8u)emberUnsignedCommandArgument(3);
  emberAfCalendarServerPublishSeasonsMessage(nodeId,
                                             srcEndpoint,
                                             dstEndpoint,
                                             calendarIndex);
}

// plugin calendar-server publish-special-days <nodeId:2> <srcEndpoint:1> <dstEndpoint:1> <calendarIndex:1>
void emAfCalendarServerCliPublishSpecialDays(void)
{
  EmberNodeId nodeId = (EmberNodeId)emberUnsignedCommandArgument(0);
  int8u srcEndpoint = (int8u)emberUnsignedCommandArgument(1);
  int8u dstEndpoint = (int8u)emberUnsignedCommandArgument(2);
  int8u calendarIndex = (int8u)emberUnsignedCommandArgument(3);
  emberAfCalendarServerPublishSpecialDaysMessage(nodeId,
                                                 srcEndpoint,
                                                 dstEndpoint,
                                                 calendarIndex);
}

// plugin calendar-server cancel-calendar <nodeId:2> <srcEndpoint:1> <dstEndpoint:1> <calendarIndex:1>
void emberAfCalendarServerCliCancelCalendar(void)
{
  EmberNodeId nodeId = (EmberNodeId)emberUnsignedCommandArgument(0);
  int8u srcEndpoint = (int8u)emberUnsignedCommandArgument(1);
  int8u dstEndpoint = (int8u)emberUnsignedCommandArgument(2);
  int8u calendarIndex = (int8u)emberUnsignedCommandArgument(3);
  emberAfCalendarServerCancelCalendarMessage(nodeId,
                                             srcEndpoint,
                                             dstEndpoint,
                                             calendarIndex);
  // now invalidate the calendar
  if (calendarIndex < EMBER_AF_PLUGIN_CALENDAR_COMMON_TOTAL_CALENDARS) {
    MEMSET(&(calendars[calendarIndex]), 0, sizeof(EmberAfCalendarStruct));
    calendars[calendarIndex].calendarId = EMBER_AF_PLUGIN_CALENDAR_COMMON_INVALID_CALENDAR_ID;
  }
}
