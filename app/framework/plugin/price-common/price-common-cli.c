// *******************************************************************
// * price-common-cli.c
// *
// *
// * Copyright 2012 by Ember Corporation. All rights reserved.              *80*
// *******************************************************************

#include "app/framework/include/af.h"
#include "app/util/serial/command-interpreter2.h"
#include "price-common.h"

//=============================================================================
// Functions

void emAfPluginPriceCommonClusterGetAdjustedStartTimeCli(void) {
  int32u startTimeUTc = (int32u)emberUnsignedCommandArgument(0);
  int8u durationType = (int8u)emberUnsignedCommandArgument(1);
  int32u adjustedStartTime;
  adjustedStartTime = emberAfPluginPriceCommonClusterGetAdjustedStartTime(startTimeUTc, 
                                                                          durationType);
  emberAfPriceClusterPrintln("adjustedStartTime: 0x%4X", adjustedStartTime);
}

void emAfPluginPriceCommonClusterConvertDurationToSecondsCli(void){
  int32u startTimeUtc = (int32u)emberUnsignedCommandArgument(0);
  int32u duration = (int32u)emberUnsignedCommandArgument(1);
  int8u durationType = (int8u)emberUnsignedCommandArgument(2);
  int32u seconds = emberAfPluginPriceCommonClusterConvertDurationToSeconds(startTimeUtc, 
                                                                           duration, 
                                                                           durationType);
  emberAfPriceClusterPrintln("seconds: %d", seconds);
}

#if !defined(EMBER_AF_GENERATE_CLI)
EmberCommandEntry emberAfPluginPriceCommonCommands[] = {
  emberCommandEntryAction("adj-st-t",
                          emAfPluginPriceCommonClusterGetAdjustedStartTimeCli,
                          "wu",
                          "Calculates a new UTC start time value based on the duration type parameter."),
  emberCommandEntryAction("cnvrt-durn-to-sec",
                          emAfPluginPriceCommonClusterConvertDurationToSecondsCli,
                          "wwu",
                          "Converts the duration to a number of seconds based on the duration type parameter."),
  emberCommandEntryTerminator(),
};
#endif
