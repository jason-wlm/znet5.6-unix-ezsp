// ****************************************************************************
// * network-steering-soc.c
// *
// * SOC specific code for Profile Interop Network Steering.
// *
// * Copyright 2014 Silicon Laboratories, Inc.                             *80*
// ****************************************************************************

#include "app/framework/include/af.h"
#include "app/framework/plugin/network-steering/network-steering.h"

//============================================================================
// Globals

static EmberMessageBuffer storedNetworks = EMBER_NULL_MESSAGE_BUFFER;

#define MAX_NETWORKS (PACKET_BUFFER_SIZE >> 1)  // 16

#define NULL_PAN_ID 0xFFFF

#define PLUGIN_NAME emAfNetworkSteeringPluginName

//============================================================================
// Forward Declarations

//============================================================================

int8u emAfPluginNetworkSteeringGetMaxPossiblePanIds(void)
{
  return MAX_NETWORKS;
}

void emAfPluginNetworkSteeringClearStoredPanIds(void)
{
  if (storedNetworks != EMBER_NULL_MESSAGE_BUFFER) {
    emberReleaseMessageBuffer(storedNetworks);
    storedNetworks = EMBER_NULL_MESSAGE_BUFFER;
  }
}

int16u* emAfPluginNetworkSteeringGetStoredPanIdPointer(int8u index)
{
  if (index >= MAX_NETWORKS) {
    return NULL;
  }

  if (storedNetworks == EMBER_NULL_MESSAGE_BUFFER) {
    storedNetworks = emberAllocateStackBuffer();
    if (storedNetworks == EMBER_NULL_MESSAGE_BUFFER) {
      emberAfCorePrintln("Error: %p failed to allocate stack buffer.", PLUGIN_NAME);
      return NULL;
    }
    MEMSET(emberMessageBufferContents(storedNetworks), 0xFF, PACKET_BUFFER_SIZE);
  }

  return (int16u*)(emberMessageBufferContents(storedNetworks) + (index * sizeof(int16u)));
}

