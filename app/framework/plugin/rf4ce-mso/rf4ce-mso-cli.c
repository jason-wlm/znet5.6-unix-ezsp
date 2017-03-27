// Copyright 2014 Silicon Laboratories, Inc.

#include "af.h"
#include "rf4ce-mso.h"

#ifdef EMBER_AF_LEGACY_CLI
  #error The RF4CE MSO plugin is not compatible with the legacy CLI.
#endif

#if defined(EMBER_AF_GENERATE_CLI) || defined(EMBER_AF_API_COMMAND_INTERPRETER2)

// plugin rf4ce-mso bind
void emberAfPluginRf4ceMsoBindCommand(void)
{
  EmberStatus status = emberAfRf4ceMsoBind();
  emberAfAppPrintln("%p 0x%x", "bind", status);
}

// plugin rf4ce-mso validate
void emberAfPluginRf4ceMsoValidateCommand(void)
{
  EmberStatus status = emberAfRf4ceMsoValidate();
  emberAfAppPrintln("%p 0x%x", "validate", status);
}

// plugin rf4ce-mso terminate
void emberAfPluginRf4ceMsoTerminateCommand(void)
{
  EmberStatus status = emberAfRf4ceMsoTerminateValidation();
  emberAfAppPrintln("%p 0x%x", "terminate", status);
}

// plugin rf4ce-mso abort <full:1>
void emberAfPluginRf4ceMsoAbortCommand(void)
{
  boolean fullAbort = (boolean)emberUnsignedCommandArgument(0);
  EmberStatus status = emberAfRf4ceMsoAbortValidation(fullAbort);
  emberAfAppPrintln("%p 0x%x", "abort", status);
}

// plugin rf4ce-mso press <pairing index:1> <rc command code:1> <rc command payload:n> <atomic:1>
void emberAfPluginRf4ceMsoPressCommand(void)
{
  EmberStatus status;
  int8u pairingIndex = (int8u)emberUnsignedCommandArgument(0);
  EmberAfRf4ceMsoKeyCode rcCommandCode
    = (EmberAfRf4ceMsoKeyCode)emberUnsignedCommandArgument(1);
  int8u rcCommandPayloadLength;
  int8u *rcCommandPayload
    = emberStringCommandArgument(2, &rcCommandPayloadLength);
  boolean atomic = (boolean)emberUnsignedCommandArgument(3);
  status = emberAfRf4ceMsoUserControlPress(pairingIndex,
                                           rcCommandCode,
                                           rcCommandPayload,
                                           rcCommandPayloadLength,
                                           atomic);
  emberAfAppPrintln("%p 0x%x", "press", status);
}

// plugin rf4ce-mso release <pairing index:1> <rc command code:1>
void emberAfPluginRf4ceMsoReleaseCommand(void)
{
  EmberStatus status;
  int8u pairingIndex = (int8u)emberUnsignedCommandArgument(0);
  EmberAfRf4ceMsoKeyCode rcCommandCode
    = (EmberAfRf4ceMsoKeyCode)emberUnsignedCommandArgument(1);
  status = emberAfRf4ceMsoUserControlRelease(pairingIndex, rcCommandCode);
  emberAfAppPrintln("%p 0x%x", "release", status);
}

// plugin rf4ce-mso set <pairing index:1> <attribute id:1> <index:1> <value:n>
void emberAfPluginRf4ceMsoSetCommand(void)
{
  EmberStatus status;
  int8u pairingIndex = (int8u)emberUnsignedCommandArgument(0);
  EmberAfRf4ceMsoAttributeId attributeId = (EmberAfRf4ceMsoAttributeId)emberUnsignedCommandArgument(1);
  int8u index = (int8u)emberUnsignedCommandArgument(2);
  int8u valueLen;
  int8u *value = emberStringCommandArgument(3, &valueLen);
  status = emberAfRf4ceMsoSetAttributeRequest(pairingIndex,
                                              attributeId,
                                              index,
                                              valueLen,
                                              value);
  emberAfAppPrintln("%p 0x%x", "set", status);
}

// plugin rf4ce-mso get <pairing index:1> <attribute id:1> <index:1> <value len:1>
void emberAfPluginRf4ceMsoGetCommand(void)
{
  EmberStatus status;
  int8u pairingIndex = (int8u)emberUnsignedCommandArgument(0);
  EmberAfRf4ceMsoAttributeId attributeId = (EmberAfRf4ceMsoAttributeId)emberUnsignedCommandArgument(1);
  int8u index = (int8u)emberUnsignedCommandArgument(2);
  int8u valueLen = (int8u)emberUnsignedCommandArgument(3);
  status = emberAfRf4ceMsoGetAttributeRequest(pairingIndex,
                                              attributeId,
                                              index,
                                              valueLen);
  emberAfAppPrintln("%p 0x%x", "get", status);
}

#endif
