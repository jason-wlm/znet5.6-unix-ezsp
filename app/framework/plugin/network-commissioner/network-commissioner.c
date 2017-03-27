//
// network-commissioner.c
//
// Andrew Keesler
//
// January 2, 2015
//
// Drives the network commissioning process as specified by
// the Base Device specification (doc 13-0402).
//
// Copyright 2014 Silicon Laboratories, Inc.                               *80*

#include "network-commissioner.h"

#ifdef EMBER_AF_PLUGIN_NETWORK_COMMISSIONER_NETWORK_STEERING
  #include EMBER_AF_API_NETWORK_STEERING
#endif

#ifdef EMBER_AF_PLUGIN_NETWORK_COMMISSIONER_NETWORK_FORMATION
  #include EMBER_AF_API_NETWORK_CREATOR
#endif

#ifdef EMBER_AF_PLUGIN_NETWORK_COMMISSIONER_FINDING_AND_BINDING
  #include EMBER_AF_API_FIND_AND_BIND
#endif

// -----------------------------------------------------------------------------
// Globals

int8u emAfPluginNetworkCommissionerEndpoint = EMBER_AF_INVALID_ENDPOINT;
extern boolean emEnableR21StackBehavior;

// -----------------------------------------------------------------------------
// Public API
void emberAfPluginNetworkCommissionerInitCallback(void)
{
  #if !defined(EZSP_HOST)
  emEnableR21StackBehavior = TRUE;
  #endif
}

void emberAfPluginNetworkCommissionerStartCommand(void)
{
  int8u endpoint = (int8u)emberUnsignedCommandArgument(0);
  emberAfPluginNetworkCommissionerStart(endpoint);
}

EmberStatus emberAfPluginNetworkCommissionerStart(int8u endpoint)
{
  EmberStatus status = EMBER_ERR_FATAL;
  EmberNodeType currentNetworkNodeType;

  // TODO: multi-network handling inside the profile interop plugins!

  /*
  // PUSH PUSH PUSH PUSH PUSH PUSH PUSH PUSH PUSH PUSH PUSH PUSH PUSH PUSH
  if ((status = emberAfPushEndpointNetworkIndex(endpoint)) != EMBER_SUCCESS) {
    emberAfCorePrintln("%p: %p %d (error = 0x%X)",
                       "Error",
                       "cannot push network index for endpoint",
                       endpoint,
                       status);
    return status;
  }
  */

  if (!emAfProIsCurrentNetwork()) {
    emberAfCorePrintln("%s: The %s plugin %s.",
                       "Error",
                       EMBER_AF_PLUGIN_NETWORK_COMMISSIONER_PLUGIN_NAME,
                       "can only perform commissioning on ZigBee PRO networks");
    return EMBER_INVALID_CALL;
  }
  
  currentNetworkNodeType = emAfCurrentZigbeeProNetwork->nodeType;
  emAfPluginNetworkCommissionerEndpoint = endpoint;

#ifdef EMBER_AF_PLUGIN_NETWORK_COMMISSIONER_TOUCHLINK_COMMISSIONING
  status = EMBER_ERR_FATAL; // TODO: perform touchlink commissioning
#endif

#ifdef EMBER_AF_PLUGIN_NETWORK_COMMISSIONER_NETWORK_STEERING
  if (status != EMBER_SUCCESS) {
    emberAfCorePrintln("%s plugin: Network steering start: 0x%X.",
                       EMBER_AF_PLUGIN_NETWORK_COMMISSIONER_PLUGIN_NAME,
                       (status = emberAfPluginNetworkSteeringStart()));
  }
#endif

#ifdef EMBER_AF_PLUGIN_NETWORK_COMMISSIONER_NETWORK_FORMATION
  if (status != EMBER_SUCCESS) {
      status
        = emberAfPluginNetworkCreatorForm(currentNetworkNodeType == EMBER_COORDINATOR);
    emberAfCorePrintln("%s plugin: Network formation start: 0x%X.",
                       EMBER_AF_PLUGIN_NETWORK_COMMISSIONER_PLUGIN_NAME,
                       status);
  }
#endif


  /*
  // POP POP POP POP POP POP POP POP POP POP POP POP POP POP POP POP POP POP
  if ((status = emberAfPopNetworkIndex()) != EMBER_SUCCESS) {
    emberAfCorePrintln("%p: cannot pop network index (error = 0x%X)",
                       "Error", status);
    return status;
  }
  */

  return status;
}

// -----------------------------------------------------------------------------
// Network Steering Callbacks

#ifdef EMBER_AF_PLUGIN_NETWORK_COMMISSIONER_NETWORK_STEERING
void emberAfPluginNetworkSteeringCompleteCallback(EmberStatus status,
                                                  int8u totalBeacons,
                                                  int8u joinAttempts,
                                                  boolean defaultKeyUsed)
{
#ifdef EMBER_AF_PLUGIN_NETWORK_COMMISSIONER_FINDING_AND_BINDING
  if (status == EMBER_SUCCESS)
    emberAfCorePrintln("%s plugin: Find and bind (initiator): 0x%X",
                       EMBER_AF_PLUGIN_NETWORK_COMMISSIONER_PLUGIN_NAME,
                       emberAfPluginFindAndBindInitiator(emAfPluginNetworkCommissionerEndpoint));
#endif /* EMBER_AF_PLUGIN_NETWORK_COMMISSIONER_FINDING_AND_BINDING */
}
#endif /* EMBER_AF_PLUGIN_NETWORK_COMMISSIONER_NETWORK_STEERING */

// -----------------------------------------------------------------------------
// Network Formation Callbacks

#ifdef EMBER_AF_PLUGIN_NETWORK_COMMISSIONER_NETWORK_FORMATION
void emberAfPluginNetworkCreatorCompleteCallback(const EmberNetworkParameters *network)
{
#ifdef EMBER_AF_PLUGIN_NETWORK_COMMISSIONER_FINDING_AND_BINDING
  emberAfCorePrintln("%s plugin: Find and bind (target): 0x%X",
                     EMBER_AF_PLUGIN_NETWORK_COMMISSIONER_PLUGIN_NAME,
                     emberAfPluginFindAndBindTarget(emAfPluginNetworkCommissionerEndpoint));
#endif // EMBER_AF_PLUGIN_NETWORK_COMMISSIONER_FINDING_AND_BINDING
}
#endif /* EMBER_AF_PLUGIN_NETWORK_COMMISSIONER_NETWORK_FORMATION */

// -----------------------------------------------------------------------------
// Finding and Binding Callbacks

#ifdef EMBER_AF_PLUGIN_NETWORK_COMMISSIONER_FINDING_AND_BINDING
void emberAfPluginFindAndBindInitiatorComplete(EmberStatus status)
{
  emberAfCorePrintln("Find and Bind: Initiator: Done: 0x%X", status);
}
#endif /* EMBER_AF_PLUGIN_NETWORK_COMMISSIONER_FINDING_AND_BINDING */

