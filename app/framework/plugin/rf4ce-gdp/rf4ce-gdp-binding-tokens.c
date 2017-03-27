// Copyright 2014 Silicon Laboratories, Inc.

#include "af.h"
#include "rf4ce-gdp.h"
#include "rf4ce-gdp-internal.h"

#include "rf4ce-gdp-attributes.h"

//------------------------------------------------------------------------------
// Token-related internal APIs.

int8u emAfRf4ceGdpGetPairingBindStatus(int8u pairingIndex)
{
  int8u status;
  halCommonGetIndexedToken(&status,
                           TOKEN_PLUGIN_RF4CE_GDP_BIND_TABLE,
                           pairingIndex);
  return status;
}

void emAfRf4ceGdpSetPairingBindStatus(int8u pairingIndex, int8u status)
{
  halCommonSetIndexedToken(TOKEN_PLUGIN_RF4CE_GDP_BIND_TABLE,
                           pairingIndex,
                           &status);
}

void emAfRf4ceGdpGetPollConfigurationAttribute(int8u pairingIndex,
                                               int8u *pollConfiguration)
{
  halCommonGetIndexedToken(pollConfiguration,
                           TOKEN_PLUGIN_RF4CE_GDP_POLLING_CONFIGURATION_TABLE,
                           pairingIndex);
}

void emAfRf4ceGdpSetPollConfigurationAttribute(int8u pairingIndex,
                                               const int8u *pollConfiguration)
{
  halCommonSetIndexedToken(TOKEN_PLUGIN_RF4CE_GDP_POLLING_CONFIGURATION_TABLE,
                           pairingIndex,
                           (int8u*)pollConfiguration);
}
