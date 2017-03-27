// ****************************************************************************
// * network-steering-host.c
// *
// * Host specific code for Profile Interop Network Steering.
// *
// * Copyright 2014 Silicon Laboratories, Inc.                             *80*
// ****************************************************************************

#include "app/framework/include/af.h"
#include "app/framework/plugin/network-steering/network-steering.h"

//============================================================================
// Globals

#define MAX_NETWORKS 16

static int16u storedNetworks[MAX_NETWORKS];
static boolean memoryCleared = FALSE;

//============================================================================
// Forward Declarations

//============================================================================

int8u emAfPluginNetworkSteeringGetMaxPossiblePanIds(void)
{
  return MAX_NETWORKS;
}

void emAfPluginNetworkSteeringClearStoredPanIds(void)
{
  memoryCleared = FALSE;
}

int16u* emAfPluginNetworkSteeringGetStoredPanIdPointer(int8u index)
{
  if (index >= MAX_NETWORKS) {
    return NULL;
  }
  if (!memoryCleared) {
    MEMSET(storedNetworks, 0xFF, sizeof(int16u) * MAX_NETWORKS);
    memoryCleared = TRUE;
  }
  return &(storedNetworks[index]);
}
