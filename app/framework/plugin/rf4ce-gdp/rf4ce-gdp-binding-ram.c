// Copyright 2014 Silicon Laboratories, Inc.

#include "af.h"
#include "rf4ce-gdp.h"
#include "rf4ce-gdp-internal.h"
#include "rf4ce-gdp-attributes.h"

static int8u bindStatusTable[EMBER_RF4CE_PAIRING_TABLE_SIZE];
static int8u pollConfigurationTable[EMBER_RF4CE_PAIRING_TABLE_SIZE][APL_GDP_POLL_CONFIGURATION_SIZE];

//------------------------------------------------------------------------------
// Token-related internal APIs.

int8u emAfRf4ceGdpGetPairingBindStatus(int8u pairingIndex)
{
  return bindStatusTable[pairingIndex];
}

void emAfRf4ceGdpSetPairingBindStatus(int8u pairingIndex, int8u status)
{
  bindStatusTable[pairingIndex] = status;
}

void emAfRf4ceGdpGetPollConfigurationAttribute(int8u pairingIndex,
                                               int8u *pollConfiguration)
{
  MEMCOPY(pollConfiguration,
          pollConfigurationTable[pairingIndex],
          APL_GDP_POLL_CONFIGURATION_SIZE);
}

void emAfRf4ceGdpSetPollConfigurationAttribute(int8u pairingIndex,
                                               const int8u *pollConfiguration)
{
  MEMCOPY(pollConfigurationTable[pairingIndex],
          pollConfiguration,
          APL_GDP_POLL_CONFIGURATION_SIZE);
}
