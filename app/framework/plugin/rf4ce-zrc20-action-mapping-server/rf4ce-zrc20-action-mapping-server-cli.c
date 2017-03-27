// Copyright 2014 Silicon Laboratories, Inc.

#include "af.h"
#include "app/framework/plugin/rf4ce-profile/rf4ce-profile.h"
#include "app/framework/plugin/rf4ce-zrc20/rf4ce-zrc20.h"
#include "rf4ce-zrc20-action-mapping-server.h"

#ifdef EMBER_AF_LEGACY_CLI
  #error The RF4CE ZRC2.0 Action Mapping Server plugin is not compatible with the legacy CLI.
#endif


// Some commands are split into multiple commands. These variables are used to
// hold the data until we get all and call the get/set command.
static EmberAfRf4ceZrcActionMapping actionMapping;
static int8u actionData[32];
static int8u irCode[32];


void emberAfPluginRf4ceZrc20ActionMappingServerSetFlagsCommand(void)
{
  actionMapping.mappingFlags = (int8u)emberUnsignedCommandArgument(0);
}

void emberAfPluginRf4ceZrc20ActionMappingServerSetRfCommand(void)
{
  actionMapping.rfConfig = (int8u)emberUnsignedCommandArgument(0);
  actionMapping.rf4ceTxOptions = (int8u)emberUnsignedCommandArgument(1);
  actionMapping.actionDataLength = emberCopyStringArgument(2,
                                                           actionData,
                                                           32,
                                                           FALSE);
  actionMapping.actionData = actionData;
}

void emberAfPluginRf4ceZrc20ActionMappingServerSetIrCommand(void)
{
  actionMapping.irConfig = (int8u)emberUnsignedCommandArgument(0);
  actionMapping.irVendorId = (int16u)emberUnsignedCommandArgument(1);
  actionMapping.irCodeLength = emberCopyStringArgument(2,
                                                       irCode,
                                                       32,
                                                       FALSE);
  actionMapping.irCode = irCode;
}

void emberAfPluginRf4ceZrc20ActionMappingServerSetCommand(void)
{
  EmberAfRf4ceZrcMappableAction mappableAction = {
    .actionDeviceType = (EmberAfRf4ceDeviceType)emberUnsignedCommandArgument(0),
    .actionBank = (EmberAfRf4ceZrcActionBank)emberUnsignedCommandArgument(1),
    .actionCode = (EmberAfRf4ceZrcActionCode)emberUnsignedCommandArgument(2)
  };
  EmberStatus status
    = emberAfRf4ceZrc20ActionMappingServerRemapAction(&mappableAction,
                                                      &actionMapping);
  emberAfAppPrintln("%p 0x%x", "set", status);
}

void emberAfPluginRf4ceZrc20ActionMappingServerResetCommand(void)
{
  EmberAfRf4ceZrcMappableAction mappableAction = {
    .actionDeviceType = (EmberAfRf4ceDeviceType)emberUnsignedCommandArgument(0),
    .actionBank = (EmberAfRf4ceZrcActionBank)emberUnsignedCommandArgument(1),
    .actionCode = (EmberAfRf4ceZrcActionCode)emberUnsignedCommandArgument(2)
  };
  EmberStatus status
    = emberAfRf4ceZrc20ActionMappingServerRestoreDefaultAction(&mappableAction);
  emberAfAppPrintln("%p 0x%x", "reset", status);
}

void emberAfPluginRf4ceZrc20ActionMappingServerResetAllCommand(void)
{
  emberAfRf4ceZrc20ActionMappingServerRestoreDefaultAllActions();
  emberAfAppPrintln("%p 0x%x", "reset-all", 0x00);
}

void emberAfPluginRf4ceZrc20ActionMappingServerGetCommand(void)
{
  int16u index;
  EmberAfRf4ceZrcMappableAction mappableAction = {
    .actionDeviceType = (EmberAfRf4ceDeviceType)emberUnsignedCommandArgument(0),
    .actionBank = (EmberAfRf4ceZrcActionBank)emberUnsignedCommandArgument(1),
    .actionCode = (EmberAfRf4ceZrcActionCode)emberUnsignedCommandArgument(2)
  };
  EmberAfRf4ceZrcActionMapping actionMapping;
  EmberStatus status;

  if (EMBER_SUCCESS ==
      (status = emAfRf4ceZrc20ActionMappingServerLookUpMappableAction(&mappableAction,
                                                                      &index))) {
    status = emAfRf4ceZrc20ActionMappingServerGetActionMapping(index,
                                                               &actionMapping);
    emberAfAppPrintln("%x", actionMapping.mappingFlags);
    if (emberAfRf4ceZrc20ActionMappingEntryHasRfDescriptor(&actionMapping)) {
      emberAfAppPrintln("%x %x",
                        actionMapping.rfConfig,
                        actionMapping.rf4ceTxOptions);
      emberAfAppPrintBuffer(actionMapping.actionData,
                            actionMapping.actionDataLength,
                            TRUE);
      emberAfAppPrintln("");
    }
    if (emberAfRf4ceZrc20ActionMappingEntryHasIrDescriptor(&actionMapping)) {
      emberAfAppPrint("%x", actionMapping.irConfig);
      if (emberAfRf4ceZrc20ActionMappingEntryHasIrVendorId(&actionMapping)) {
          emberAfAppPrintln(" %2x", actionMapping.irVendorId);
      } else {
        emberAfAppPrintln("");
      }
      emberAfAppPrintBuffer(actionMapping.irCode,
                            actionMapping.irCodeLength,
                            TRUE);
      emberAfAppPrintln("");
    }
  }
  emberAfAppPrintln("%p 0x%x", "get", status);
}


