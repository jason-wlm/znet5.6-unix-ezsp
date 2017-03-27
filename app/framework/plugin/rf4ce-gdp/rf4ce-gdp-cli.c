// Copyright 2014 Silicon Laboratories, Inc.

#include "af.h"
#include "rf4ce-gdp.h"

#ifdef EMBER_AF_LEGACY_CLI
  #error The RF4CE GDP plugin is not compatible with the legacy CLI.
#endif

#if defined(EMBER_AF_GENERATE_CLI) || defined(EMBER_AF_API_COMMAND_INTERPRETER2)

// plugin rf4ce-gdp push-button <set pending:1>
void emberAfPluginRf4ceGdpPushButtonCommand(void)
{
  boolean setPending = (boolean)emberUnsignedCommandArgument(0);
  emberAfRf4ceGdpPushButton(setPending);
}

// plugin rf4ce-gdp set-validation <success:1>
void emberAfPluginRf4ceGdpSetValidationCommand(void)
{
  boolean success = (boolean)emberUnsignedCommandArgument(0);
  emberAfRf4ceGdpSetValidationStatus(success
                                     ? EMBER_AF_RF4CE_GDP_CHECK_VALIDATION_STATUS_SUCCESS
                                     : EMBER_AF_RF4CE_GDP_CHECK_VALIDATION_STATUS_FAILURE);
}

// plugin rf4ce-gdp initiate-key-exchange <pairing index:1>
void emberAfPluginRf4ceGdpInitiateKeyExchangeCommand(void)
{
  int8u pairingIndex = (int8u)emberUnsignedCommandArgument(0);
  EmberStatus status = emberAfRf4ceGdpInitiateKeyExchange(pairingIndex);
  emberAfAppPrintln("%p 0x%x", "initiate", status);
}

// plugin rf4ce-gdp push-attribute <pairing index:1> <profile id:1> <vendor id:2> <attribute id:1> <entry id:2> <value:n>
void emberAfPluginRf4ceGdpPushAttributeCommand(void)
{
  EmberAfRf4ceGdpAttributeRecord record;
  int8u pairingIndex = (int8u)emberUnsignedCommandArgument(0);
  int8u profileId = (int8u)emberUnsignedCommandArgument(1);
  int16u vendorId = (int16u)emberUnsignedCommandArgument(2);
  record.attributeId = (EmberAfRf4ceGdpAttributeId)emberUnsignedCommandArgument(3);
  record.entryId = (int16u)emberUnsignedCommandArgument(4);
  record.value = emberStringCommandArgument(5, &record.valueLength);
  EmberStatus status = emberAfRf4ceGdpPushAttributes(pairingIndex,
                                                     profileId,
                                                     vendorId,
                                                     &record,
                                                     1); // one record
  emberAfAppPrintln("%p 0x%x", "push", status);
}

// plugin rf4ce-gdp identify <pairing index:1> <vendor id:2> <flags:1> <time:2>
void emberAfPluginRf4ceGdpIdentifyCommand(void)
{
  int8u pairingIndex = (int8u)emberUnsignedCommandArgument(0);
  int16u vendorId = (int16u)emberUnsignedCommandArgument(1);
  EmberAfRf4ceGdpClientNotificationIdentifyFlags flags = (EmberAfRf4ceGdpClientNotificationIdentifyFlags)emberUnsignedCommandArgument(2);
  int16u timeS = (int16u)emberUnsignedCommandArgument(3);
  EmberStatus status = emberAfRf4ceGdpClientNotificationIdentify(pairingIndex,
                                                                 vendorId,
                                                                 flags,
                                                                 timeS);
  emberAfAppPrintln("%p 0x%x", "identify", status);
}

#endif
