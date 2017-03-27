//
// network-steering-cli.c
//
// Author: Andrew Keesler
//
// CLI Command functions for the network-steering plugin.
//
// Copyright 2014 Silicon Laboratories, Inc.                               *80*

#include "af.h"
#include "network-steering.h"

#if defined(EMBER_AF_GENERATE_CLI) || defined(EMBER_AF_API_COMMAND_INTERPRETER2)

// -----------------------------------------------------------------------------
// Helper macros and functions

static void addOrSubtractChannel(int8u maskToAddTo,
                                 int8u channelToAdd,
                                 boolean operationIsAdd)
{
  if (channelToAdd < EMBER_MIN_802_15_4_CHANNEL_NUMBER
      || channelToAdd > EMBER_MAX_802_15_4_CHANNEL_NUMBER) {
    emberAfCorePrintln("Channel not valid: %d", channelToAdd);
  } else if (maskToAddTo == 1) { 
    if (operationIsAdd)
      SETBIT(emAfPluginNetworkSteeringPrimaryChannelMask, channelToAdd);
    else
      CLEARBIT(emAfPluginNetworkSteeringPrimaryChannelMask, channelToAdd);
    
    emberAfCorePrintln("%p mask now 0x%4X",
                       "Primary", 
                       emAfPluginNetworkSteeringPrimaryChannelMask);      
  } else if (maskToAddTo == 2) {
    if (operationIsAdd)
      SETBIT(emAfPluginNetworkSteeringSecondaryChannelMask, channelToAdd);
    else
      CLEARBIT(emAfPluginNetworkSteeringSecondaryChannelMask, channelToAdd);
    
    emberAfCorePrintln("%p mask now 0x%4X",
                       "Secondary", 
                       emAfPluginNetworkSteeringSecondaryChannelMask);
  } else {
    emberAfCorePrintln("Mask not valid: %d", maskToAddTo);
  }
}

// -----------------------------------------------------------------------------
// Command definitions

// plugin network-steering channel add <[1=primary|2=secondary]:1> <channel:1>
// plugin network-steering channel subtract <[1=primary|2=secondary]:1> <channel:1>
void emberAfPluginNetworkSteeringChannelAddOrSubtractCommand(void)
{
  boolean operationIsAdd = (emberStringCommandArgument(-1, NULL)[0] == 'a');
  int8u maskToAddTo  = (int8u)emberUnsignedCommandArgument(0);
  int8u channelToAdd = (int8u)emberUnsignedCommandArgument(1);

  addOrSubtractChannel(maskToAddTo,
                       channelToAdd,
                       operationIsAdd);
}

// plugin network-steering status
void emberAfPluginNetworkSteeringStatusCommand(void)
{
  emberAfCorePrintln("%p status:", emAfNetworkSteeringPluginName);

  emberAfCorePrintln("Channel mask:");
  emberAfCorePrint("    (1) 0x%4X [",
                   emAfPluginNetworkSteeringPrimaryChannelMask);
  emberAfPrintChannelListFromMask(emAfPluginNetworkSteeringPrimaryChannelMask);
  emberAfCorePrintln("]");
  emberAfCorePrint("    (2) 0x%4X [",
                   emAfPluginNetworkSteeringSecondaryChannelMask);
  emberAfPrintChannelListFromMask(emAfPluginNetworkSteeringSecondaryChannelMask);
  emberAfCorePrintln("]");

  emberAfCorePrintln("State: 0x%X (%p)",
                     emAfPluginNetworkSteeringState,
                     emAfPluginNetworkSteeringStateNames[emAfPluginNetworkSteeringState]);
  emberAfCorePrintln("Pan ID index: %d",
                     emAfPluginNetworkSteeringPanIdIndex);
  emberAfCorePrintln("Current channel: %d",
                     emAfPluginNetworkSteeringCurrentChannel);
  emberAfCorePrintln("Total beacons: %d",
                     emAfPluginNetworkSteeringTotalBeacons);
  emberAfCorePrintln("Join attempts: %d",
                     emAfPluginNetworkSteeringJoinAttempts);
  emberAfCorePrintln("Network state: 0x%X",
                     emberAfNetworkState());
}

#endif /* 
          defined(EMBER_AF_GENERATE_CLI)
          || defined(EMBER_AF_API_COMMAND_INTERPRETER2)
       */
