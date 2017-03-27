//
// network-creator.c
//
// Author: Andrew Keesler
//
// Network creation process as specified by the Base Device Behavior
// specification.
//
// Copyright 2014 Silicon Laboratories, Inc.                               *80*

#include "app/framework/include/af.h"
#include "app/framework/util/af-main.h" // emAfFormNetwork

#include "network-creator.h"
#include "network-creator-composite.h"
#include "network-creator-security.h"

#define EM_AF_PLUGIN_NETWORK_CREATOR_DEBUG

#ifdef EMBER_TEST
  #ifndef EMBER_AF_PLUGIN_NETWORK_CREATOR_SCAN_DURATION
    #define EMBER_AF_PLUGIN_NETWORK_CREATOR_SCAN_DURATION 0x04
  #endif
  #ifndef EMBER_AF_PLUGIN_NETWORK_CREATOR_CHANNEL_MASK
    #define EMBER_AF_PLUGIN_NETWORK_CREATOR_CHANNEL_MASK 0x02108800
  #endif
  #ifndef EMBER_AF_PLUGIN_NETWORK_CREATOR_RADIO_POWER
    #define EMBER_AF_PLUGIN_NETWORK_CREATOR_RADIO_POWER 3
  #endif
  #define HIDDEN
#else
  #define HIDDEN static
#endif

#ifdef EM_AF_PLUGIN_NETWORK_CREATOR_DEBUG
  #define debugPrintln(...) emberAfCorePrintln(__VA_ARGS__)
#else
  #define debugPrintln(...)
#endif

// -----------------------------------------------------------------------------
// Globals

int32u emAfPluginNetworkCreatorPrimaryChannelMask = EMBER_AF_PLUGIN_NETWORK_CREATOR_CHANNEL_MASK;

// The Base Device spec (13-0402) says, by default, define the secondary channel mask to
// be all channels XOR the primary mask. See 6.2, table 2.
#define SECONDARY_CHANNEL_MASK                            \
  (EMBER_ALL_802_15_4_CHANNELS_MASK ^ EMBER_AF_PLUGIN_NETWORK_CREATOR_CHANNEL_MASK)
int32u emAfPluginNetworkCreatorSecondaryChannelMask = (int32u)SECONDARY_CHANNEL_MASK;

HIDDEN EmAfPluginNetworkCreatorChannelComposite channelComposites[EMBER_NUM_802_15_4_CHANNELS];

// State.
enum {
  STATE_READY                      = 0,

  // Bit 2 = 0.
  STATE_ACTIVE_SCAN_PRIMARY_MASK   = 1,
  STATE_ENERGY_SCAN_PRIMARY_MASK   = 2,
  STATE_FORM_PRIMARY_MASK          = 3,

  // Bit 2 = 1.
  STATE_ACTIVE_SCAN_SECONDARY_MASK = 4,
  STATE_ENERGY_SCAN_SECONDARY_MASK = 5,
  STATE_FORM_SECONDARY_MASK        = 6,

  STATE_FAIL                       = 7,
};
typedef int8u NetworkCreatorStateMask;
HIDDEN NetworkCreatorStateMask networkCreatorState = STATE_READY;
#define maskIsSecondary() (networkCreatorState & 0x4)

// TODO: remove this guy when testing is over.
boolean emAfPluginNetworkCreatorUseProfileInteropTestKey = FALSE;

// TODO: this should just become a compile time guy when development is over.
// This runtime variable is just to make switching from centralized to distributed
// testing easier.
#ifdef EMBER_AF_PLUGIN_NETWORK_CREATOR_CENTRALIZED_TRUST_CENTER
static boolean doFormCentralizedNetwork = TRUE;
#else
static boolean doFormCentralizedNetwork = FALSE;
#endif

// -----------------------------------------------------------------------------
// Declarations
  
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#define resetChannelMasks()                         \
  emAfPluginNetworkCreatorPrimaryChannelMask        \
    = EMBER_AF_PLUGIN_NETWORK_CREATOR_CHANNEL_MASK; \
  emAfPluginNetworkCreatorSecondaryChannelMask      \
    = SECONDARY_CHANNEL_MASK;

static void cleanChannelComposites(void);
static void maybeClearChannelBitOfMaxRssiReading(int8u channel,
                                                 int32u *channelMask);
/* state machine */
static EmberStatus stateMachineRun(void);
static EmberStatus startScan(void);
static void updateChannelComposites(int8s rssi,
                                    EmAfPluginNetworkCreatorChannelCompositeMetric metric,
                                    int8u channel);
static void cleanupAndStop(EmberStatus status);

// -----------------------------------------------------------------------------
// Public API Definitions

EmberStatus emberAfPluginNetworkCreatorForm(boolean centralizedNetwork)
{
  if (networkCreatorState != STATE_READY
      || (emberAfNetworkState() != EMBER_NO_NETWORK)) {
    debugPrintln("%p: Cannot start. State: %d",
                 EMBER_AF_NETWORK_CREATOR_PLUGIN_NAME,
                 networkCreatorState);
    return EMBER_INVALID_CALL;
  }

  doFormCentralizedNetwork = centralizedNetwork;

  // Reset channel masks.
  // TODO: take this out if we change channel masks to compile time only thingys.
  resetChannelMasks();

  cleanChannelComposites();

  return stateMachineRun();
}

void emberAfPluginNetworkCreatorStop()
{
  cleanupAndStop(EMBER_ERR_FATAL);
}

// -----------------------------------------------------------------------------
// Callbacks

void emberAfNetworkFoundCallback(EmberZigbeeNetwork *networkFound,
                                 int8u lqi,
                                 int8s rssi)
{
  debugPrintln("Found network!");
  debugPrintln("  PanId: 0x%2X, Channel: %d, PJoin: %p",
                     networkFound->panId,
                     networkFound->channel,
                     (networkFound->allowingJoin ? "YES" : "NO"));
  debugPrintln("  lqi:  %d", lqi);
  debugPrintln("  rssi: %d", rssi);

  updateChannelComposites(rssi,
                          EM_AF_PLUGIN_NETWORK_CREATOR_CHANNEL_COMPOSITE_METRIC_BEACONS,
                          networkFound->channel);
}

void emberAfEnergyScanResultCallback(int8u channel, int8s rssi)
{
  debugPrintln("Energy scan results.");
  debugPrintln("%p: Channel: %d. Rssi: %d",
               EMBER_AF_NETWORK_CREATOR_PLUGIN_NAME,
               channel,
               rssi);
  
  updateChannelComposites(rssi,
                          EM_AF_PLUGIN_NETWORK_CREATOR_CHANNEL_COMPOSITE_METRIC_RSSI,
                          channel);
}

void emberAfScanCompleteCallback(int8u channel, EmberStatus status)
{
  debugPrintln("%p: Scan complete. Channel: %d. Status: 0x%X",
               EMBER_AF_NETWORK_CREATOR_PLUGIN_NAME,
               channel,
               status);

  if (status == EMBER_SUCCESS) {
    // If this was an active scan, then move to an energy scan.
    // Else it was an energy scan, so note that you are done with
    // scanning.
    stateMachineRun();
  } else {
    // Just turn off the channel on which the scan failed. The network-creator
    // will disregard this channel in the network formation process.
    if (maskIsSecondary())
      CLEARBIT(emAfPluginNetworkCreatorSecondaryChannelMask, channel);
    else
      CLEARBIT(emAfPluginNetworkCreatorPrimaryChannelMask, channel);
  }
}

void emberAfScanErrorCallback(EmberStatus status)
{
  debugPrintln("%p: Scan error. Status: 0x%X",
               EMBER_AF_NETWORK_CREATOR_PLUGIN_NAME,
               status);  

  cleanupAndStop(status);
}

void emberAfSecurityInitCallback(EmberInitialSecurityState *state,
                                 EmberExtendedSecurityBitmask *extended,
                                 boolean trustCenter)
{
  if (!state)
    return;
  
  // Set the global link key, if necessary.
  if (emAfPluginNetworkCreatorUseProfileInteropTestKey) {
    EmberKeyData centralizedKey
      = PROFILE_INTEROP_CENTRALIZED_KEY;
    EmberKeyData distributedKey
      = PROFILE_INTEROP_DISTRIBUTED_KEY;

    if (doFormCentralizedNetwork)
      MEMCOPY(emberKeyContents(&(state->preconfiguredKey)),
              emberKeyContents(&(centralizedKey)),
              EMBER_ENCRYPTION_KEY_SIZE);
    else
      MEMCOPY(emberKeyContents(&(state->preconfiguredKey)),
              emberKeyContents(&(distributedKey)),
              EMBER_ENCRYPTION_KEY_SIZE);      
  }
}

// Contains internal logic for the channel on which to form a network.
EmberStatus emAfPluginNetworkCreatorShouldFormNetwork(void)
{
  EmberNetworkParameters networkParameters;
  EmberStatus status;
  int8u channel
    = (halCommonGetRandom() & 0x0F) + EMBER_MIN_802_15_4_CHANNEL_NUMBER;
  int32u *channelMask
    = (maskIsSecondary()
       ? &emAfPluginNetworkCreatorSecondaryChannelMask
       : &emAfPluginNetworkCreatorPrimaryChannelMask);

  status = EMBER_ERR_FATAL;
  while (*channelMask && status != EMBER_SUCCESS) {
    // Find the next channel on which to try forming a network.
    while (!(*channelMask & BIT32(channel)))
      channel = (channel == EMBER_MAX_802_15_4_CHANNEL_NUMBER
                 ? EMBER_MIN_802_15_4_CHANNEL_NUMBER
                 : channel + 1);

    // Try to form a network.
    networkParameters.panId = halCommonGetRandom();
    networkParameters.radioTxPower = EMBER_AF_PLUGIN_NETWORK_CREATOR_RADIO_POWER;
    networkParameters.radioChannel = channel;
    status = emAfFormNetwork(&networkParameters, doFormCentralizedNetwork);
    debugPrintln("%p: Form. Channel: %d. Status: 0x%X",
                 EMBER_AF_NETWORK_CREATOR_PLUGIN_NAME,
                 channel,
                 status);
    
    // If you pass, then ask the user what they want to do.
    if (status == EMBER_SUCCESS) {
      if (emberAfPluginNetworkCreatorShouldFormNetworkCallback(channel,
                                                               maskIsSecondary()))
        emberAfPluginNetworkCreatorCompleteCallback(&networkParameters);
      else
        status = EMBER_ERR_FATAL;
    }

    // Clear the channel bit that you just tried.
    CLEARBIT(*channelMask, channel);
  }

  return status;
}

// -----------------------------------------------------------------------------
// Private API Definitions

static EmberStatus stateMachineRun(void)
{
  EmberStatus status;

  networkCreatorState ++;
  
  debugPrintln("%p: State machine run. State: %d",
               EMBER_AF_NETWORK_CREATOR_PLUGIN_NAME,
               networkCreatorState);

  // Should we be scanning?
  if (networkCreatorState != STATE_FORM_SECONDARY_MASK
      && networkCreatorState != STATE_FORM_PRIMARY_MASK)
    status = startScan();
  // If not, then we should be forming.
  else if (emAfPluginNetworkCreatorShouldFormNetwork() == EMBER_SUCCESS)
    cleanupAndStop((status = EMBER_SUCCESS));
  // Do we still have states left to go through?
  else if (networkCreatorState + 1 >= STATE_FAIL)
    status = EMBER_ERR_FATAL;
  // Then go through the next states.
  else {
    cleanChannelComposites();
    status = stateMachineRun();
  }

  if (status != EMBER_SUCCESS)
    cleanupAndStop(status);

  debugPrintln("%p: State machine return. State: %d",
               EMBER_AF_NETWORK_CREATOR_PLUGIN_NAME,
               networkCreatorState);

  return status;
}

static EmberStatus startScan(void)
{
  EmberStatus status;

  status = emberStartScan(((networkCreatorState == STATE_ACTIVE_SCAN_PRIMARY_MASK
                            || networkCreatorState == STATE_ACTIVE_SCAN_SECONDARY_MASK)
                           ? EMBER_ACTIVE_SCAN
                           : EMBER_ENERGY_SCAN),
                          (maskIsSecondary()
                           ? emAfPluginNetworkCreatorSecondaryChannelMask
                           : emAfPluginNetworkCreatorPrimaryChannelMask),
                          EMBER_AF_PLUGIN_NETWORK_CREATOR_SCAN_DURATION);

  debugPrintln("%p: Scan. Status: 0x%X",
               EMBER_AF_NETWORK_CREATOR_PLUGIN_NAME,
               status);

  return status;
}

static void updateChannelComposites(int8s rssi,
                                    EmAfPluginNetworkCreatorChannelCompositeMetric metric,
                                    int8u channel)
{
  int32u *channelMask
    = (maskIsSecondary()
       ? &emAfPluginNetworkCreatorSecondaryChannelMask
       : &emAfPluginNetworkCreatorPrimaryChannelMask);
  int8u channelIndex
    = channel - EMBER_MIN_802_15_4_CHANNEL_NUMBER;
  
  // If the channel bit is off, then we are disregarding this channel.
  if (!READBIT(*channelMask, channel))
      return;

  // Update the network composite value for this channel.
  switch(metric) {
  case EM_AF_PLUGIN_NETWORK_CREATOR_CHANNEL_COMPOSITE_METRIC_BEACONS:
    channelComposites[channelIndex].beaconsHeard ++;
    break;

  case EM_AF_PLUGIN_NETWORK_CREATOR_CHANNEL_COMPOSITE_METRIC_RSSI:
    if (rssi > channelComposites[channelIndex].maxRssiHeard) {
      channelComposites[channelIndex].maxRssiHeard = rssi;
      maybeClearChannelBitOfMaxRssiReading(channel, channelMask);
    }
    break;

  default:
    debugPrintln("Unknown metric value: %d", metric);
  }

  // If the channel is over the composite threshold, disregard the channel.
  if (emAfPluginNetworkCreatorChannelCompositeIsAboveThreshold(channelComposites[channelIndex]))
    CLEARBIT(*channelMask, channel);
}

static void cleanupAndStop(EmberStatus status)
{
  debugPrintln("%p: Stop. Status: 0x%X. State: %d",
               EMBER_AF_NETWORK_CREATOR_PLUGIN_NAME,
               status,
               networkCreatorState);

  networkCreatorState = STATE_READY;
}

static void cleanChannelComposites(void)
{
  int8u i;
  for (i = 0; i < EMBER_NUM_802_15_4_CHANNELS; i ++) {
    channelComposites[i].beaconsHeard
      = 0;
    channelComposites[i].maxRssiHeard
      = EM_AF_PLUGIN_NETWORK_CREATOR_CHANNEL_COMPOSITE_INVALID_RSSI;
  }
}

static void maybeClearChannelBitOfMaxRssiReading(int8u channel, int32u *channelMask)
{
  int8u i, channelsConsideredSoFar, maxIndex;
  
  // Find max RSSI index and how many channels have been considered so far.
  for (i = 0, channelsConsideredSoFar = 0, maxIndex = 0;
       i < EMBER_NUM_802_15_4_CHANNELS;
       i ++) {
    // If we have received an RSSI reading from this channel,
    // and we are still considering this channel...
    if ((channelComposites[i].maxRssiHeard
         != EM_AF_PLUGIN_NETWORK_CREATOR_CHANNEL_COMPOSITE_INVALID_RSSI)
        && (READBIT(*channelMask, i + EMBER_MIN_802_15_4_CHANNEL_NUMBER))) {
      
      // ...increment the channelsConsideredSoFar by 1 and see if this
      // channel has the maximum RSSI.
      channelsConsideredSoFar ++;
      if (channelComposites[i].maxRssiHeard
          > channelComposites[maxIndex].maxRssiHeard) {
        maxIndex = i;    
      }
    }
  }

  // If the number of channels considered so far is more than the number of
  // channels that you want to consider, then remove the channel with the
  // maximum RSSI.
  if ((channelsConsideredSoFar
       > EM_AF_PLUGIN_NETWORK_CREATOR_CHANNELS_TO_CONSIDER)) {

    CLEARBIT(*channelMask,
             ((channelComposites[channel - EMBER_MIN_802_15_4_CHANNEL_NUMBER].maxRssiHeard
               > channelComposites[maxIndex].maxRssiHeard)
              ? channel
              : maxIndex + EMBER_MIN_802_15_4_CHANNEL_NUMBER));
  }
}
