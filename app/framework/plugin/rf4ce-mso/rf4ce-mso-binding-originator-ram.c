// Copyright 2014 Silicon Laboratories, Inc.

#include "af.h"
#include "rf4ce-mso.h"
#include "rf4ce-mso-internal.h"

#ifdef EMBER_AF_PLUGIN_RF4CE_MSO_IS_ORIGINATOR

// This variable stores the pairing index of the current active bind, if any.
static int8u activeBindPairingIndex = 0xFF;

int8u emAfRf4ceMsoGetActiveBindPairingIndex(void)
{
  return activeBindPairingIndex;
}

void emAfRf4ceMsoSetActiveBindPairingIndex(int8u pairingIndex)
{
  activeBindPairingIndex = pairingIndex;
}

#endif
