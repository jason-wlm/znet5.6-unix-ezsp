// *******************************************************************
// * multi-network.c
// *
// *
// * Copyright 2014 Silicon Laboratories, Inc.                              *80*
// *******************************************************************

#include "app/framework/include/af.h"

PGM EmAfNetworkType *emAfCurrentNetworkType = NULL;
PGM EmAfZigbeeProNetwork *emAfCurrentZigbeeProNetwork = NULL;

//#define NETWORK_INDEX_DEBUG
#if defined(EMBER_TEST) || defined(NETWORK_INDEX_DEBUG)
  #define NETWORK_INDEX_ASSERT(x) assert(x)
#else
  #define NETWORK_INDEX_ASSERT(x)
#endif

#if EMBER_SUPPORTED_NETWORKS == 1
EmberStatus emAfInitializeNetworkIndexStack(void)
{
  NETWORK_INDEX_ASSERT(emberGetCurrentNetwork() == 0);
  NETWORK_INDEX_ASSERT(EMBER_AF_DEFAULT_NETWORK_INDEX == 0);
  emAfCurrentNetworkType = &emAfNetworkTypes[0];
  emAfCurrentZigbeeProNetwork = &emAfZigbeeProNetworks[0];
  return EMBER_SUCCESS;
}

EmberStatus emberAfPushNetworkIndex(int8u networkIndex)
{
  NETWORK_INDEX_ASSERT(networkIndex == 0);
  NETWORK_INDEX_ASSERT(emberGetCurrentNetwork() == 0);
  return (networkIndex == 0 ? EMBER_SUCCESS : EMBER_INVALID_CALL);
}

EmberStatus emberAfPushCallbackNetworkIndex(void)
{
  NETWORK_INDEX_ASSERT(emberGetCurrentNetwork() == 0);
  NETWORK_INDEX_ASSERT(emberGetCallbackNetwork() == 0);
  return EMBER_SUCCESS;
}

EmberStatus emberAfPushEndpointNetworkIndex(int8u endpoint)
{
  int8u networkIndex = emberAfNetworkIndexFromEndpoint(endpoint);
  NETWORK_INDEX_ASSERT(networkIndex != 0xFF);
  if (networkIndex == 0xFF) {
    return EMBER_INVALID_ENDPOINT;
  }
  NETWORK_INDEX_ASSERT(networkIndex == 0);
  NETWORK_INDEX_ASSERT(emberGetCurrentNetwork() == 0);
  return EMBER_SUCCESS;
}

EmberStatus emberAfPopNetworkIndex(void)
{
  NETWORK_INDEX_ASSERT(emberGetCurrentNetwork() == 0);
  return EMBER_SUCCESS;
}

void emAfAssertNetworkIndexStackIsEmpty(void)
{
  NETWORK_INDEX_ASSERT(emberGetCurrentNetwork() == 0);
}

int8u emberAfPrimaryEndpointForNetworkIndex(int8u networkIndex)
{
  NETWORK_INDEX_ASSERT(networkIndex == 0);
  return (networkIndex == 0 ? emberAfPrimaryEndpoint() : 0xFF);
}

int8u emberAfPrimaryEndpointForCurrentNetworkIndex(void)
{
  NETWORK_INDEX_ASSERT(emberGetCurrentNetwork() == 0);
  return emberAfPrimaryEndpoint();
}

int8u emberAfNetworkIndexFromEndpoint(int8u endpoint)
{
  int8u index = emberAfIndexFromEndpoint(endpoint);
  NETWORK_INDEX_ASSERT(index != 0xFF);
  if (index == 0xFF) {
    return 0xFF;
  }
  NETWORK_INDEX_ASSERT(emberAfNetworkIndexFromEndpointIndex(index) == 0);
  return 0;
}

void emberAfNetworkEventControlSetInactive(EmberEventControl *controls)
{
  NETWORK_INDEX_ASSERT(emberGetCurrentNetwork() == 0);
  emberEventControlSetInactive(controls[0]);
}

boolean emberAfNetworkEventControlGetActive(EmberEventControl *controls)
{
  NETWORK_INDEX_ASSERT(emberGetCurrentNetwork() == 0);
  return emberEventControlGetActive(controls[0]);
}

void emberAfNetworkEventControlSetActive(EmberEventControl *controls)
{
  NETWORK_INDEX_ASSERT(emberGetCurrentNetwork() == 0);
  emberEventControlSetActive(controls[0]);
}

EmberStatus emberAfNetworkEventControlSetDelayMS(EmberEventControl *controls,
                                                 int32u delayMs)
{
  NETWORK_INDEX_ASSERT(emberGetCurrentNetwork() == 0);
  return emberAfEventControlSetDelayMS(&controls[0], delayMs);
}

EmberStatus emberAfNetworkEventControlSetDelayQS(EmberEventControl *controls,
                                                 int32u delayQs)
{
  NETWORK_INDEX_ASSERT(emberGetCurrentNetwork() == 0);
  return emberAfEventControlSetDelayQS(&controls[0], delayQs);
}

EmberStatus emberAfNetworkEventControlSetDelayMinutes(EmberEventControl *controls,
                                                      int16u delayM)
{
  NETWORK_INDEX_ASSERT(emberGetCurrentNetwork() == 0);
  return emberAfEventControlSetDelayMinutes(&controls[0], delayM);
}

#else

// We use two bits to describe a network index and sixteen bits to store our
// stack of network indices.  This limits us to a maximum of four networks
// indices and a maximum of eight in our stack.  We also remember one default
// network that we resort to when our stack is empty.
static int16u networkIndexStack = 0;
static int8u networkIndices = 0;
#define NETWORK_INDEX_BITS       2
#define NETWORK_INDEX_MAX        (1 << NETWORK_INDEX_BITS)
#define NETWORK_INDEX_MASK       (NETWORK_INDEX_MAX - 1)
#define NETWORK_INDEX_STACK_SIZE (sizeof(networkIndexStack) * 8 / NETWORK_INDEX_BITS)

static EmberStatus setCurrentNetwork(void)
{
  EmberStatus status;
  int8u networkIndex = (networkIndices == 0
                        ? EMBER_AF_DEFAULT_NETWORK_INDEX
                        : networkIndexStack & NETWORK_INDEX_MASK);
  status = emberSetCurrentNetwork(networkIndex);
  NETWORK_INDEX_ASSERT(status == EMBER_SUCCESS);
  NETWORK_INDEX_ASSERT(emberGetCurrentNetwork() == networkIndex);
  if (status == EMBER_SUCCESS) {
    emAfCurrentNetworkType = &emAfNetworkTypes[networkIndex];
    if (emAfNetworkTypes[networkIndex] == EM_AF_NETWORK_TYPE_ZIGBEE_PRO) {
      int8u i, proNetworkIndex = 0;
      for (i = 0; i < networkIndex; i++) {
        if (emAfNetworkTypes[i] == EM_AF_NETWORK_TYPE_ZIGBEE_PRO) {
          proNetworkIndex++;
        }
      }
      emAfCurrentZigbeeProNetwork = &emAfZigbeeProNetworks[proNetworkIndex];
    } else {
      emAfCurrentZigbeeProNetwork = NULL;
    }
  }
  return status;
}

EmberStatus emAfInitializeNetworkIndexStack(void)
{
  EmberStatus status;
  NETWORK_INDEX_ASSERT(networkIndices == 0);
  if (networkIndices != 0) {
    return EMBER_INVALID_CALL;
  }
  status = setCurrentNetwork();
  NETWORK_INDEX_ASSERT(status == EMBER_SUCCESS);
  NETWORK_INDEX_ASSERT(networkIndices == 0);
  NETWORK_INDEX_ASSERT(emberGetCurrentNetwork() == EMBER_AF_DEFAULT_NETWORK_INDEX);
  return status;
}

EmberStatus emberAfPushNetworkIndex(int8u networkIndex)
{
  EmberStatus status;
  NETWORK_INDEX_ASSERT(networkIndex < NETWORK_INDEX_MAX);
  if (NETWORK_INDEX_MAX <= networkIndex) {
    return EMBER_INDEX_OUT_OF_RANGE;
  }
  NETWORK_INDEX_ASSERT(networkIndices < NETWORK_INDEX_STACK_SIZE);
  if (NETWORK_INDEX_STACK_SIZE <= networkIndices) {
    return EMBER_TABLE_FULL;
  }
  networkIndexStack <<= NETWORK_INDEX_BITS;
  networkIndexStack |= networkIndex;
  networkIndices++;
  status = setCurrentNetwork();
  NETWORK_INDEX_ASSERT(status == EMBER_SUCCESS);
  NETWORK_INDEX_ASSERT(0 < networkIndices);
  NETWORK_INDEX_ASSERT(emberGetCurrentNetwork() == networkIndex);
  return status;
}

EmberStatus emberAfPushCallbackNetworkIndex(void)
{
  EmberStatus status = emberAfPushNetworkIndex(emberGetCallbackNetwork());
  NETWORK_INDEX_ASSERT(status == EMBER_SUCCESS);
  NETWORK_INDEX_ASSERT(0 < networkIndices);
  NETWORK_INDEX_ASSERT(emberGetCurrentNetwork() == emberGetCallbackNetwork());
  return status;
}

EmberStatus emberAfPushEndpointNetworkIndex(int8u endpoint)
{
  EmberStatus status;
  int8u networkIndex = emberAfNetworkIndexFromEndpoint(endpoint);
  NETWORK_INDEX_ASSERT(networkIndex != 0xFF);
  if (networkIndex == 0xFF) {
    return EMBER_INVALID_ENDPOINT;
  }
  status = emberAfPushNetworkIndex(networkIndex);
  NETWORK_INDEX_ASSERT(status == EMBER_SUCCESS);
  NETWORK_INDEX_ASSERT(0 < networkIndices);
  NETWORK_INDEX_ASSERT(emberGetCurrentNetwork() == networkIndex);
  return status;
}

EmberStatus emberAfPopNetworkIndex(void)
{
  EmberStatus status;
  NETWORK_INDEX_ASSERT(0 < networkIndices);
  if (networkIndices == 0) {
    return EMBER_INVALID_CALL;
  }
  networkIndexStack >>= NETWORK_INDEX_BITS;
  networkIndices--;
  status = setCurrentNetwork();
  NETWORK_INDEX_ASSERT(status == EMBER_SUCCESS);
  return status;
}

void emAfAssertNetworkIndexStackIsEmpty(void)
{
  NETWORK_INDEX_ASSERT(networkIndices == 0);
  NETWORK_INDEX_ASSERT(emberGetCurrentNetwork() == EMBER_AF_DEFAULT_NETWORK_INDEX);
}

int8u emberAfPrimaryEndpointForNetworkIndex(int8u networkIndex)
{
  int8u i;
  NETWORK_INDEX_ASSERT(networkIndex < NETWORK_INDEX_MAX);
  for (i = 0; i < emberAfEndpointCount(); i++) {
    if (emberAfNetworkIndexFromEndpointIndex(i) == networkIndex) {
      return emberAfEndpointFromIndex(i);
    }
  }
  return 0xFF;
}

int8u emberAfPrimaryEndpointForCurrentNetworkIndex(void)
{
  return emberAfPrimaryEndpointForNetworkIndex(emberGetCurrentNetwork());
}

int8u emberAfNetworkIndexFromEndpoint(int8u endpoint)
{
  int8u index = emberAfIndexFromEndpoint(endpoint);
  NETWORK_INDEX_ASSERT(index != 0xFF);
  return (index == 0xFF ? 0xFF : emberAfNetworkIndexFromEndpointIndex(index));
}

void emberAfNetworkEventControlSetInactive(EmberEventControl *controls)
{
  EmberEventControl *control = controls + emberGetCurrentNetwork();
  emberEventControlSetInactive(*control);
}

boolean emberAfNetworkEventControlGetActive(EmberEventControl *controls)
{
  EmberEventControl *control = controls + emberGetCurrentNetwork();
  return emberEventControlGetActive(*control);
}

void emberAfNetworkEventControlSetActive(EmberEventControl *controls)
{
  EmberEventControl *control = controls + emberGetCurrentNetwork();
  emberEventControlSetActive(*control);
}

EmberStatus emberAfNetworkEventControlSetDelayMS(EmberEventControl *controls,
                                                 int32u delayMs)
{
  EmberEventControl *control = controls + emberGetCurrentNetwork();
  return emberAfEventControlSetDelayMS(control, delayMs);
}

EmberStatus emberAfNetworkEventControlSetDelayQS(EmberEventControl *controls,
                                                 int32u delayQs)
{
  EmberEventControl *control = controls + emberGetCurrentNetwork();
  return emberAfEventControlSetDelayQS(control, delayQs);
}

EmberStatus emberAfNetworkEventControlSetDelayMinutes(EmberEventControl *controls,
                                                      int16u delayM)
{
  EmberEventControl *control = controls + emberGetCurrentNetwork();
  return emberAfEventControlSetDelayMinutes(control, delayM);
}

#endif
