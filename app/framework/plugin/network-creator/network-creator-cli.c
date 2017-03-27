//
// network-creator-cli.c
//
// Author: Andrew Keesler
//
// Copyright 2014 Silicon Laboratories, Inc.                               *80*
//

#include "app/framework/include/af.h"

#include "network-creator.h"
#include "network-creator-security.h"

#if defined(EMBER_AF_GENERATE_CLI) || defined(EMBER_AF_API_COMMAND_INTERPRETER2)

// -----------------------------------------------------------------------------
// CLI Command Definitions

// TODO: take out the second parameter because it is supposed to be
// a compile time thing.
// plugin network-creator form <power:1> <centralized:1>
void emberAfPluginNetworkCreatorFormCommand(void)
{
  emberAfCorePrintln("%p: Form: 0x%X",
                     EMBER_AF_NETWORK_CREATOR_PLUGIN_NAME,
                     emberAfPluginNetworkCreatorForm(
                                     (boolean)emberUnsignedCommandArgument(0)));
}

// TODO: if we decide to make these channel masks compile time only,
// then we will take out this command.
// plugin network-creator mask add <mask:1> <channel:1>
// plugin network-creator mask subtract <mask:1> <channel:1>
// plugin network-creator mask set <mask:1> <channel:1>
void emberAfPluginNetworkCreatorChannelMaskCommand(void)
{
  boolean channelMaskIsPrimary = ((int8u)emberUnsignedCommandArgument(0) == 1);
  int32u channelOrNewMask = (int32u)emberUnsignedCommandArgument(1);
  int32u *channelMask = (channelMaskIsPrimary
                         ? &emAfPluginNetworkCreatorPrimaryChannelMask
                         : &emAfPluginNetworkCreatorSecondaryChannelMask);

  // Check if operation is add or subtract first.
  if (emberStringCommandArgument(-1, NULL)[1] != 'e') {
    if (channelOrNewMask < EMBER_MIN_802_15_4_CHANNEL_NUMBER
        || channelOrNewMask > EMBER_MAX_802_15_4_CHANNEL_NUMBER)
      emberAfCorePrintln("Illegal 802.15.4 channel: %d", channelOrNewMask);
    else if (emberStringCommandArgument(-1, NULL)[0] == 'a')
      *channelMask |= (1 << channelOrNewMask);
    else
      *channelMask &= ~(1 << channelOrNewMask);
  } else {
    *channelMask = channelOrNewMask;
  }
    
  emberAfCorePrint("%p channel mask now: 0x%4X [",
                     (channelMaskIsPrimary ? "Primary" : "Secondary"),
                     *channelMask);
  emberAfPrintChannelListFromMask(*channelMask);
  emberAfCorePrintln("]");
}

// plugin network-creator status
void emberAfPluginNetworkCreatorStatusCommand(void)
{
  emberAfCorePrintln("Channel mask:");
  emberAfCorePrint("    (1) 0x%4X [",
                   emAfPluginNetworkCreatorPrimaryChannelMask);
  emberAfPrintChannelListFromMask(emAfPluginNetworkCreatorPrimaryChannelMask);
  emberAfCorePrintln("]");
  emberAfCorePrint("    (2) 0x%4X [",
                   emAfPluginNetworkCreatorSecondaryChannelMask);
  emberAfPrintChannelListFromMask(emAfPluginNetworkCreatorSecondaryChannelMask);
  emberAfCorePrintln("]");
}

// plugin network-creator toggle
void emberAfPluginNetworkCreatorToggleCommand(void)
{
  emAfPluginNetworkCreatorUseProfileInteropTestKey
    = !emAfPluginNetworkCreatorUseProfileInteropTestKey;
  emberAfCorePrintln("Use the profile interop test key: %s",
                     (emAfPluginNetworkCreatorUseProfileInteropTestKey
                      ? "YES"
                      : "NO"));
}

#endif /* 
          defined(EMBER_AF_GENERATE_CLI)
          || defined(EMBER_AF_API_COMMAND_INTERPRETER2)
       */

