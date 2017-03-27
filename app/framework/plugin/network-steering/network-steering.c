// ****************************************************************************
// * network-steering.c
// *
// * This plugin will perform the Profile interop method of steering and
// * finding a network to join.
// *
// * Copyright 2014 Silicon Laboratories, Inc.                             *80*
// ****************************************************************************

// General Procedure

// 1. Setup stack security with an Install code key (if available)
//    If not available, skip to step 9.
// 2. Set the channel mask to the Primary set of channels.
// 3. Perform a scan of open networks on a single channel.
// 4. For all beacons collected on that channel, filter out PAN IDs that are not open
//    for joining or don't have the right stack profile (2), or are duplicates.
// 5. For each discovered network, try to join.
// 6. Repeat steps 3-5 until all channels in the mask are tried.
// 7. If no networks are successfully joined, setup secondary channel mask 
//    (all channels) and repeat steps 3-6 with.
// 8. If no networks are successfully joined, change the security
//    to use the default well-known "ZigbeeAlliance09" key.
// 9. Repeat steps 2-7
//    (This will again try primary channel mask first, then secondary).
// 10. If no networks are joined successfully fail the whole process and return
//     control to the application.

#include "app/framework/include/af.h"
#include "app/framework/plugin/network-steering/network-steering.h"

//============================================================================
// Globals

#if !defined(EMBER_AF_PLUGIN_NETWORK_STEERING_CHANNEL_MASK)
  #define EMBER_AF_PLUGIN_NETWORK_STEERING_CHANNEL_MASK \
    (BIT32(11) | BIT32(14))
#endif

#if !defined(EMBER_AF_PLUGIN_NETWORK_STEERING_SCAN_DURATION)
  #define EMBER_AF_PLUGIN_NETWORK_STEERING_SCAN_DURATION 5
#endif

PGM_P emAfPluginNetworkSteeringStateNames[] = {
  "None",
  "Scan Primary Channels and use Install Code",
  "Scan Secondary Channels and use Install Code",
  "Scan Pirmary Channels and Use Default Key",
  "Scan Secondary Channels and Use Default Key",
};

#define LAST_STATE EMBER_AF_NETWORK_STEERING_STATE_SCAN_SECONDARY_FOR_DEFAULT_KEY

EmberAfPluginNetworkSteeringJoiningState emAfPluginNetworkSteeringState = EMBER_AF_NETWORK_STEERING_STATE_NONE;

#define SECURITY_BITMASK (EMBER_HAVE_PRECONFIGURED_KEY  \
                          | EMBER_REQUIRE_ENCRYPTED_KEY \
                          | EMBER_NO_FRAME_COUNTER_RESET)

PGM int8u emAfNetworkSteeringPluginName[] = "NWK Steering";
#define PLUGIN_NAME emAfNetworkSteeringPluginName

#define PLUGIN_DEBUG
#if defined(PLUGIN_DEBUG)
  #define debugPrintln(...) emberAfCorePrintln(__VA_ARGS__)
  #define debugPrint(...)   emberAfCorePrint(__VA_ARGS__)
  #define debugExec(x) do { x; } while(0)
#else
  #define debugPrintln(...)
  #define debugPrint(...)
  #define debugExec(x)
#endif

#define SECONDARY_CHANNEL_MASK EMBER_ALL_802_15_4_CHANNELS_MASK

#define REQUIRED_STACK_PROFILE 2

static const EmberKeyData defaultLinkKey = {
  { 0x5A, 0x69, 0x67, 0x42, 0x65, 0x65, 0x41, 0x6C,
    0x6C, 0x69, 0x61, 0x6E, 0x63, 0x65, 0x30, 0x39
  },
};

static boolean printedMaxPanIdsWarning = FALSE;
int8u emAfPluginNetworkSteeringPanIdIndex = 0;
int8u emAfPluginNetworkSteeringCurrentChannel;

// We make these into variables so that they can be changed at run time.
// This is very useful for unit and interop tests.
int32u emAfPluginNetworkSteeringPrimaryChannelMask = EMBER_AF_PLUGIN_NETWORK_STEERING_CHANNEL_MASK;
int32u emAfPluginNetworkSteeringSecondaryChannelMask = SECONDARY_CHANNEL_MASK;

int8u emAfPluginNetworkSteeringTotalBeacons = 0;
int8u emAfPluginNetworkSteeringJoinAttempts = 0;

static int32u currentChannelMask = 0;

//============================================================================
// Forward Declarations

static void cleanupAndStop(EmberStatus status);
static EmberStatus stateMachineRun(void);
static int8u getNextChannel(void);
static void tryNextMethod(void);
static EmberStatus setupSecurity(void);

// TODO:  Regenerating the test headers should make this unnecessary
int8s emberAfPluginNetworkSteeringGetPowerForRadioChannelCallback(int8u channel);

//============================================================================

static boolean addPanIdCandidate(int16u panId)
{
  int16u* panIdPointer = emAfPluginNetworkSteeringGetStoredPanIdPointer(0);
  if (panIdPointer == NULL) {
    emberAfCorePrintln("Error: %p could not get memory pointer for stored PAN IDs",
                       PLUGIN_NAME);
    return FALSE;
  }
  int8u maxNetworks = emAfPluginNetworkSteeringGetMaxPossiblePanIds();
  int8u i;
  for (i = 0; i < maxNetworks; i++) {
    if (panId == *panIdPointer) {
      // We already know about this PAN, no point in recording it twice.
      debugPrintln("Ignoring duplicate PAN ID 0x%2X", panId);
      return TRUE;
    } else if (*panIdPointer == EMBER_AF_INVALID_PAN_ID) {
      *panIdPointer = panId;
      debugPrintln("Stored PAN ID 0x%2X", *panIdPointer);
      return TRUE;
    }
    panIdPointer++;
  }

  if (!printedMaxPanIdsWarning) {
    emberAfCorePrintln("Warning: %p Max PANs reached (%d)",
                       PLUGIN_NAME,
                       maxNetworks);
    printedMaxPanIdsWarning = TRUE;
  }
  return TRUE;
}

static void clearPanIdCandidates(void)
{
  printedMaxPanIdsWarning = FALSE;
  emAfPluginNetworkSteeringClearStoredPanIds();
  emAfPluginNetworkSteeringPanIdIndex = 0;
}

static int16u getNextCandidate(void)
{
  debugPrintln("Getting candidate at index %d", emAfPluginNetworkSteeringPanIdIndex);
  int16u* pointer =
    emAfPluginNetworkSteeringGetStoredPanIdPointer(emAfPluginNetworkSteeringPanIdIndex);
  if (pointer == NULL) {
    debugPrintln("Error: %p could not get pointer to stored PANs", PLUGIN_NAME);
    return EMBER_AF_INVALID_PAN_ID;
  }
  emAfPluginNetworkSteeringPanIdIndex++;
  return *pointer;
}

void gotoNextChannel(void)
{
  emAfPluginNetworkSteeringCurrentChannel = getNextChannel();
  if (emAfPluginNetworkSteeringCurrentChannel == 0) {
    debugPrintln("No more channels");
    tryNextMethod();
    return;
  }
  clearPanIdCandidates();
  EmberStatus status = emberStartScan(EMBER_ACTIVE_SCAN,
                                      BIT32(emAfPluginNetworkSteeringCurrentChannel),
                                      EMBER_AF_PLUGIN_NETWORK_STEERING_SCAN_DURATION);
  if (EMBER_SUCCESS != status) {
    emberAfCorePrintln("Error: %p start scan failed: 0x%X", PLUGIN_NAME, status);
    cleanupAndStop(status);
  } else {
    emberAfCorePrintln("Starting scan on channel %d",
                       emAfPluginNetworkSteeringCurrentChannel);
  }
}

void tryToJoinNetwork(void)
{
  EmberNetworkParameters networkParams;
  MEMSET(&networkParams, 0, sizeof(EmberNetworkParameters));

  networkParams.panId = getNextCandidate();
  if (networkParams.panId == EMBER_AF_INVALID_PAN_ID) {
    debugPrintln("No networks to join on channel %d", emAfPluginNetworkSteeringCurrentChannel);
    gotoNextChannel();
    return;
  }

  emberAfCorePrintln("%p joining 0x%2x", PLUGIN_NAME, networkParams.panId);
  networkParams.radioChannel = emAfPluginNetworkSteeringCurrentChannel;
  networkParams.radioTxPower = emberAfPluginNetworkSteeringGetPowerForRadioChannelCallback(emAfPluginNetworkSteeringCurrentChannel);
  EmberStatus status = emberAfJoinNetwork(&networkParams);
  emAfPluginNetworkSteeringJoinAttempts++;
  if (EMBER_SUCCESS != status) {
    emberAfCorePrintln("Error: %p could not attempt join: 0x%X", status);
    cleanupAndStop(status);
    return;
  }
}

void emberAfPluginNetworkSteeringStackStatusCallback(EmberStatus status)
{
  if (emAfPluginNetworkSteeringState == EMBER_AF_NETWORK_STEERING_STATE_NONE) {
    return;
  }

  if (status == EMBER_NETWORK_UP) {
    emberAfCorePrintln("%p network joined.  Steering complete.", PLUGIN_NAME);
    cleanupAndStop(EMBER_SUCCESS);
    return;
  }

  tryToJoinNetwork();
}

void emberAfScanCompleteCallback(int8u channel, EmberStatus status)
{
  if (status != EMBER_SUCCESS) {
    emberAfCorePrintln("Error: Scan complete handler returned 0x%X", status);
    cleanupAndStop(status);
    return;
  }
  tryToJoinNetwork();
}

void emberAfNetworkFoundCallback(EmberZigbeeNetwork *networkFound,
                                 int8u lqi,
                                 int8s rssi)
{
  emAfPluginNetworkSteeringTotalBeacons++;

  if (!(networkFound->allowingJoin
        && networkFound->stackProfile == REQUIRED_STACK_PROFILE)) {
    return;
  }

  debugPrint("%p nwk found ch: %d, panID 0x%2X, xpan: ",
             PLUGIN_NAME,
             networkFound->channel,
             networkFound->panId);
  debugExec(emberAfPrintBigEndianEui64(networkFound->extendedPanId));
  debugPrintln("");

  if (!addPanIdCandidate(networkFound->panId)) {
    emberAfCorePrintln("Error: %p could not add candidate network.",
                       PLUGIN_NAME);
    cleanupAndStop(EMBER_ERR_FATAL);
    return;
  }
}

static void tryNextMethod(void)
{
  emAfPluginNetworkSteeringState++;
  if (emAfPluginNetworkSteeringState > LAST_STATE) {
    cleanupAndStop(emAfPluginNetworkSteeringTotalBeacons > 0
                   ? EMBER_JOIN_FAILED
                   : EMBER_NO_BEACONS);
    return;
  }
  stateMachineRun();
}

static void cleanupAndStop(EmberStatus status)
{
  emberAfCorePrintln("%p Stop.  Cleaning up.", PLUGIN_NAME);
  emAfPluginNetworkSteeringState = EMBER_AF_NETWORK_STEERING_STATE_NONE;
  emAfPluginNetworkSteeringClearStoredPanIds();
  emberAfPluginNetworkSteeringCompleteCallback(status,
                                               emAfPluginNetworkSteeringTotalBeacons,
                                               emAfPluginNetworkSteeringJoinAttempts,
                                               (emAfPluginNetworkSteeringState
                                                > EMBER_AF_NETWORK_STEERING_STATE_SCAN_PRIMARY_FOR_DEFAULT_KEY));
  emAfPluginNetworkSteeringPanIdIndex = 0;
  emAfPluginNetworkSteeringJoinAttempts = 0;
  emAfPluginNetworkSteeringTotalBeacons = 0;
}

void emberAfScanErrorCallback(EmberStatus status)
{
  emberAfCorePrintln("%p Scan Error, status: 0x%X", PLUGIN_NAME, status);
  cleanupAndStop(status);
}

static int8u getNextChannel(void)
{
  if (emAfPluginNetworkSteeringCurrentChannel == 0) {
    emAfPluginNetworkSteeringCurrentChannel = (halCommonGetRandom() & 0x0F)
                                               + EMBER_MIN_802_15_4_CHANNEL_NUMBER;
    debugPrintln("Randomly choosing a starting channel %d.", emAfPluginNetworkSteeringCurrentChannel);
  } else {
    emAfPluginNetworkSteeringCurrentChannel++;
  }
  while (currentChannelMask != 0) {
    if (BIT32(emAfPluginNetworkSteeringCurrentChannel) & currentChannelMask) {
      currentChannelMask &= ~(BIT32(emAfPluginNetworkSteeringCurrentChannel));
      return emAfPluginNetworkSteeringCurrentChannel;
    }
    emAfPluginNetworkSteeringCurrentChannel++;
    if (emAfPluginNetworkSteeringCurrentChannel > EMBER_MAX_802_15_4_CHANNEL_NUMBER) {
      emAfPluginNetworkSteeringCurrentChannel = EMBER_MIN_802_15_4_CHANNEL_NUMBER;
    }
  }
  return 0;
}

static EmberStatus stateMachineRun(void)
{
  EmberStatus status = EMBER_SUCCESS;
  emberAfCorePrintln("%p State: %p",
                     PLUGIN_NAME,
                     emAfPluginNetworkSteeringStateNames[emAfPluginNetworkSteeringState]);

  switch (emAfPluginNetworkSteeringState) {
    case EMBER_AF_NETWORK_STEERING_STATE_SCAN_PRIMARY_FOR_INSTALL_CODE:
    case EMBER_AF_NETWORK_STEERING_STATE_SCAN_PRIMARY_FOR_DEFAULT_KEY: {
      EmberStatus status = setupSecurity();
      if (EMBER_SUCCESS != status) {
        emberAfCorePrintln("Error: Could not set security: 0x%X", status);
        cleanupAndStop(status);
        return status;
      }
      currentChannelMask = emAfPluginNetworkSteeringPrimaryChannelMask;
      break;
    }
    default:
      currentChannelMask = emAfPluginNetworkSteeringSecondaryChannelMask;
      break;
  }

  debugPrintln("Channel Mask: 0x%4X", currentChannelMask);
  gotoNextChannel();
  return status;
}

static EmberStatus setupSecurity(void)
{
  EmberInitialSecurityState securityState;
  MEMSET(&securityState, 0, sizeof(EmberInitialSecurityState));
  MEMCOPY(emberKeyContents(&(securityState.preconfiguredKey)),
          emberKeyContents(&defaultLinkKey),
          EMBER_ENCRYPTION_KEY_SIZE);
  EmberStatus status;
  boolean useInstallCode
    = ((emAfPluginNetworkSteeringState
        == EMBER_AF_NETWORK_STEERING_STATE_SCAN_SECONDARY_FOR_INSTALL_CODE)
       || (emAfPluginNetworkSteeringState
           == EMBER_AF_NETWORK_STEERING_STATE_SCAN_PRIMARY_FOR_INSTALL_CODE));

  securityState.bitmask = (SECURITY_BITMASK
// Disabled for now until we have full support for retrieving 
// new link key on joining.
//                           | EMBER_GET_LINK_KEY_WHEN_JOINING
                           | (useInstallCode
                              ? EMBER_GET_PRECONFIGURED_KEY_FROM_INSTALL_CODE
                              : 0));

  debugPrintln("%p Security Bitmask: 0x%2X",
               PLUGIN_NAME,
               securityState.bitmask);
  status = emberSetInitialSecurityState(&securityState);
  debugPrintln("%p Set Initial Security State: 0x%X",
               PLUGIN_NAME,
               status);

  return status;
}

EmberStatus emberAfPluginNetworkSteeringStart(void)
{
  if (emAfPluginNetworkSteeringState != EMBER_AF_NETWORK_STEERING_STATE_NONE
      || emberAfNetworkState() != EMBER_NO_NETWORK) {
    return EMBER_INVALID_CALL;
  }

  emAfPluginNetworkSteeringState
    = EMBER_AF_NETWORK_STEERING_STATE_SCAN_PRIMARY_FOR_INSTALL_CODE;

  return stateMachineRun();
}

EmberStatus emberAfPluginNetworkSteeringStop(void)
{
  if (emAfPluginNetworkSteeringState == EMBER_AF_NETWORK_STEERING_STATE_NONE) {
    return EMBER_INVALID_CALL;
  }
  cleanupAndStop(EMBER_JOIN_FAILED);
  return EMBER_SUCCESS;
}
