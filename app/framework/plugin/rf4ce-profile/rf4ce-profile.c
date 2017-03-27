// Copyright 2014 Silicon Laboratories, Inc.

#include "af.h"
#include "rf4ce-profile.h"
#include "rf4ce-profile-internal.h"

//#define NETWORK_INDEX_DEBUG
#if defined(EMBER_TEST) || defined(NETWORK_INDEX_DEBUG)
  #define NETWORK_INDEX_ASSERT(x) assert(x)
#else
  #define NETWORK_INDEX_ASSERT(x)
#endif

#ifdef EMBER_AF_RF4CE_NODE_TYPE_CONTROLLER
  #define NWKC_MIN_PAIRING_TABLE_SIZE NWKC_MIN_CONTROLLER_PAIRING_TABLE_SIZE
#else
  #define NWKC_MIN_PAIRING_TABLE_SIZE NWKC_MIN_TARGET_PAIRING_TABLE_SIZE
#endif
#if EMBER_RF4CE_PAIRING_TABLE_SIZE < NWKC_MIN_PAIRING_TABLE_SIZE
  #error The device does not support the minimum number of pairing table entries for the node type.
#endif

#define DISCOVERY_IN_PROGRESS() (discoveryProfileIdCount != 0)
#define SET_DISCOVERY_IN_PROGRESS(count, list) \
  do {                                         \
    discoveryProfileIdCount = (count);         \
    MEMMOVE(discoveryProfileIds, (list), discoveryProfileIdCount); \
  } while (FALSE)
#define UNSET_DISCOVERY_IN_PROGRESS() (discoveryProfileIdCount = 0)
#define AUTO_DISCOVERY_IN_PROGRESS() (autoDiscoveryProfileIdCount != 0)
#define SET_AUTO_DISCOVERY_IN_PROGRESS(count, list)                        \
  do {                                                                     \
    autoDiscoveryProfileIdCount = (count);                                 \
    MEMMOVE(autoDiscoveryProfileIds, (list), autoDiscoveryProfileIdCount); \
  } while (FALSE)
#define UNSET_AUTO_DISCOVERY_IN_PROGRESS() (autoDiscoveryProfileIdCount = 0)
#define PAIRING_IN_PROGRESS() (pairing)
#define SET_PAIRING_IN_PROGRESS() (pairing = TRUE)
#define UNSET_PAIRING_IN_PROGRESS() (pairing = FALSE)
#define UNPAIRING_IN_PROGRESS() (unpairing)
#define SET_UNPAIRING_IN_PROGRESS() (unpairing = TRUE)
#define UNSET_UNPAIRING_IN_PROGRESS() (unpairing = FALSE)

#define SET_PAIRING_INDEX(pairingIndex) (currentPairingIndex = (pairingIndex))
#define UNSET_PAIRING_INDEX() (currentPairingIndex = 0xFF)

EmberRf4ceVendorInfo emAfRf4ceVendorInfo = EMBER_AF_RF4CE_VENDOR_INFO;
EmberRf4ceApplicationInfo emAfRf4ceApplicationInfo = EMBER_AF_RF4CE_APPLICATION_INFO;

static int8u discoveryProfileIds[EMBER_RF4CE_APPLICATION_PROFILE_ID_LIST_MAX_LENGTH];
static int8u discoveryProfileIdCount;
static int8u autoDiscoveryProfileIds[EMBER_RF4CE_APPLICATION_PROFILE_ID_LIST_MAX_LENGTH];
static int8u autoDiscoveryProfileIdCount;
static boolean pairing;
static boolean unpairing;

static int8u currentPairingIndex;

static int8u nextMessageTag = 0;

// The following variables keep track of whether the application and the
// various profiles want the receiver on.  The default state is that the
// application wants the receiver on (via the power-saving state {0, 0}) but
// the profiles do not (via the default FALSEs in
// emAfRf4ceRxOnWhenIdleProfileStates[]).
PGM EmberAfRf4ceProfileId emAfRf4ceProfileIds[] = EMBER_AF_RF4CE_PROFILE_IDS;
boolean emAfRf4ceRxOnWhenIdleProfileStates[EMBER_AF_RF4CE_PROFILE_ID_COUNT];
EmAfRf4cePowerSavingState emAfRf4cePowerSavingState = {0, 0}; // rx enabled

#ifdef EMBER_MULTI_NETWORK_STRIPPED
  EmberStatus emberAfPushNetworkIndex(int8u networkIndex)
  {
    return EMBER_SUCCESS;
  }

  EmberStatus emberAfPushCallbackNetworkIndex(void)
  {
    return EMBER_SUCCESS;
  }

  EmberStatus emberAfPopNetworkIndex(void)
  {
    return EMBER_SUCCESS;
  }

  boolean emberAfRf4ceIsCurrentNetwork(void)
  {
    return TRUE;
  }

  EmberStatus emberAfRf4cePushNetworkIndex(void)
  {
    return EMBER_SUCCESS;
  }

  static void setRf4ceNetwork(void)
  {
  }
#else
  static int8u rf4ceNetworkIndex = 0xFF;

  boolean emberAfRf4ceIsCurrentNetwork(void)
  {
    NETWORK_INDEX_ASSERT(rf4ceNetworkIndex != 0xFF);
    return (emberGetCurrentNetwork() == rf4ceNetworkIndex);
  }

  EmberStatus emberAfRf4cePushNetworkIndex(void)
  {
    NETWORK_INDEX_ASSERT(rf4ceNetworkIndex != 0xFF);
    return emberAfPushNetworkIndex(rf4ceNetworkIndex);
  }

  static void setRf4ceNetwork(void)
  {
    int8u i;
    for (i = 0; i < EMBER_SUPPORTED_NETWORKS; i++) {
      if (emAfNetworkTypes[i] == EM_AF_NETWORK_TYPE_ZIGBEE_RF4CE) {
        NETWORK_INDEX_ASSERT(rf4ceNetworkIndex == 0xFF);
        rf4ceNetworkIndex = i;
      }
    }
    NETWORK_INDEX_ASSERT(rf4ceNetworkIndex != 0xFF);
  }
#endif

void emberAfPluginRf4ceProfileInitCallback(void)
{
  setRf4ceNetwork();

  UNSET_DISCOVERY_IN_PROGRESS();
  UNSET_AUTO_DISCOVERY_IN_PROGRESS();
  UNSET_PAIRING_IN_PROGRESS();
  UNSET_UNPAIRING_IN_PROGRESS();

  UNSET_PAIRING_INDEX();

  emAfRf4ceSetApplicationInfo(&emAfRf4ceApplicationInfo);
}

EmberStatus emberAfRf4ceStart(void)
{
  EmberStatus status = emberAfRf4cePushNetworkIndex();
  if (status == EMBER_SUCCESS) {
    status = emAfRf4ceSetApplicationInfo(&emAfRf4ceApplicationInfo);
    if (status == EMBER_SUCCESS) {
      status = emAfRf4ceStart(EMBER_AF_RF4CE_NODE_CAPABILITIES,
                              &emAfRf4ceVendorInfo,
                              EMBER_AF_RF4CE_POWER);
    }
    emberAfPopNetworkIndex();
  }
  return status;
}

EmberStatus emberAfRf4ceStop(void)
{
  EmberStatus status = emberAfRf4cePushNetworkIndex();
  if (status == EMBER_SUCCESS) {
    status = emAfRf4ceStop();
    emberAfPopNetworkIndex();
  }
  return status;
}

int8u emberAfRf4ceGetMaxPayload(int8u pairingIndex,
                                EmberRf4ceTxOption txOptions)
{
  EmberStatus status = emberAfRf4cePushNetworkIndex();
  int8u maxLength = 0;

  if (status == EMBER_SUCCESS) {
    maxLength = emAfRf4ceGetMaxPayload(pairingIndex, txOptions);
    emberAfPopNetworkIndex();
  }

  return maxLength;
}

static boolean rxEnabled(void)
{
  // The receiver should be enabled if the power saving is disabled (i.e., the
  // duty cycle is zero) or if any of the profiles wants the receiver on.
  // Otherwise, the receiver can be turned off, either until further notice or
  // using power saving.  The wildcard profile is used to set the default state
  // for the application, so it is not checked along with the other profiles.
  int8u i;
  if (emAfRf4cePowerSavingState.dutyCycleMs == 0) {
    return TRUE;
  }
  for (i = 0; i < EMBER_AF_RF4CE_PROFILE_ID_COUNT; i++) {
    if (emAfRf4ceProfileIds[i] != EMBER_AF_RF4CE_PROFILE_WILDCARD
        && emAfRf4ceRxOnWhenIdleProfileStates[i]) {
      return TRUE;
    }
  }
  return FALSE;
}

static EmberStatus rxEnable(EmberAfRf4ceProfileId profileId, boolean enable)
{
  // Each profile can indicate whether it needs the receiver on or not and the
  // wildcard profile can be used to set the default state for the application.
  // Specifying enabled for the wildcard profile simply disables power saving
  // completely and will leave the receiver on.  Conversely, setting the
  // wildcard profile to disabled will allow the receiver to be turned off when
  // no other profile needs it to be on.  Because the wildcard profile is used
  // to set the default state for the application, it is not set like the other
  // profiles are.
  int8u i;
  if (profileId == EMBER_AF_RF4CE_PROFILE_WILDCARD) {
    emAfRf4cePowerSavingState.dutyCycleMs = (enable ? 0 : 1);
    emAfRf4cePowerSavingState.activePeriodMs = 0;
    return EMBER_SUCCESS;
  }
  for (i = 0; i < EMBER_AF_RF4CE_PROFILE_ID_COUNT; i++) {
    // A check for the wildcard profile is not required here because of the
    // return statement in the if statement above.
    if (emAfRf4ceProfileIds[i] == profileId) {
      emAfRf4ceRxOnWhenIdleProfileStates[i] = enable;
      return EMBER_SUCCESS;
    }
  }
  return EMBER_INVALID_CALL;
}

static EmberStatus setPowerSavingParameters(boolean wasEnabled)
{
  EmberStatus status = emberAfRf4cePushNetworkIndex();
  if (status == EMBER_SUCCESS) {
    // If the receiver was enabled and is staying enabled, we have nothing to
    // do.  Otherwise, enable the receiver if we're now on or set the power-
    // saving parameters if not.  If the receiver doesn't have to be enabled,
    // we always set the power-saving parameters even if the receiver wasn't
    // enabled before.  This is because we may have got here via a call to
    // emberAfRf4ceSetPowerSavingParameters that set new parameters.  This
    // ensures the parameters set in the stack are up to date.
    if (!rxEnabled()) {
      status = emAfRf4ceSetPowerSavingParameters(emAfRf4cePowerSavingState.dutyCycleMs,
                                                 emAfRf4cePowerSavingState.activePeriodMs);
    } else if (!wasEnabled) {
      status = emAfRf4ceSetPowerSavingParameters(0, 0); // enable
    }
    emberAfPopNetworkIndex();
  }
  return status;
}

EmberStatus emberAfRf4ceSetPowerSavingParameters(int32u dutyCycleMs,
                                                 int32u activePeriodMs)
{
  boolean wasEnabled;

  // This duplicates the check in emberRf4ceSetPowerSavingParameters in the
  // stack.  We need to be able to validate the parameters without actually
  // setting them in the stack.  This can happen when the application is
  // setting new parameters at the same time that some other profile needs the
  // receiver to be on.  In that case, the receiver should stay on until the
  // profile is finished and only then should the device enter power-saving
  // mode.  We want to validate and then remember the parameters so that we can
  // use them later without worrying about what to do if they were bogus.
  if (dutyCycleMs != 0 && activePeriodMs != 0
      && (dutyCycleMs > EMBER_RF4CE_MAX_DUTY_CYCLE_MS
          || activePeriodMs < EMBER_RF4CE_MIN_ACTIVE_PERIOD_MS
          || activePeriodMs >= dutyCycleMs)) {
    return EMBER_INVALID_CALL;
  }

  // Check the current global state, save the new power-saving parameters, then
  // set the new global state.
  wasEnabled = rxEnabled();
  emAfRf4cePowerSavingState.dutyCycleMs = dutyCycleMs;
  emAfRf4cePowerSavingState.activePeriodMs = activePeriodMs;
  return setPowerSavingParameters(wasEnabled);
}

EmberStatus emberAfRf4ceRxEnable(EmberAfRf4ceProfileId profileId,
                                 boolean enable)
{
  // Check the current global state, set the profile state, then set the new
  // global state.
  boolean wasEnabled = rxEnabled();
  EmberStatus status = rxEnable(profileId, enable);
  if (status == EMBER_SUCCESS) {
    status = setPowerSavingParameters(wasEnabled);
  }
  return status;
}

EmberStatus emberAfRf4ceSetFrequencyAgilityParameters(int8u rssiWindowSize,
                                                      int8u channelChangeReads,
                                                      int8s rssiThreshold,
                                                      int16u readIntervalS,
                                                      int8u readDuration)
{
  EmberStatus status = emberAfRf4cePushNetworkIndex();
  if (status == EMBER_SUCCESS) {
    status = emAfRf4ceSetFrequencyAgilityParameters(rssiWindowSize,
                                                    channelChangeReads,
                                                    rssiThreshold,
                                                    readIntervalS,
                                                    readDuration);
    emberAfPopNetworkIndex();
  }
  return status;
}

EmberStatus emberAfRf4ceDiscovery(EmberPanId panId,
                                  EmberNodeId nodeId,
                                  int8u searchDevType,
                                  int16u discDuration,
                                  int8u maxDiscRepetitions,
                                  int8u discProfileIdListLength,
                                  int8u *discProfileIdList)
{
  EmberStatus status = EMBER_INVALID_CALL;
  if (!DISCOVERY_IN_PROGRESS()) {
    status = emberAfRf4cePushNetworkIndex();
    if (status == EMBER_SUCCESS) {
      status = emAfRf4ceDiscovery(panId,
                                  nodeId,
                                  searchDevType,
                                  discDuration,
                                  maxDiscRepetitions,
                                  discProfileIdListLength,
                                  discProfileIdList);
      if (status == EMBER_SUCCESS) {
        SET_DISCOVERY_IN_PROGRESS(discProfileIdListLength, discProfileIdList);
      }
      emberAfPopNetworkIndex();
    }
  }
  return status;
}

boolean emAfRf4ceDiscoveryRequestHandler(EmberEUI64 srcIeeeAddr,
                                         int8u nodeCapabilities,
                                         const EmberRf4ceVendorInfo *vendorInfo,
                                         const EmberRf4ceApplicationInfo *appInfo,
                                         int8u searchDevType,
                                         int8u rxLinkQuality)
{
  int8u i;
  int8u profileIdListLength = emberAfRf4ceProfileIdListLength(appInfo->capabilities);
  boolean retVal = FALSE;

  emberAfPushCallbackNetworkIndex();

  for (i = 0; i < profileIdListLength && !retVal; i++) {
    switch (appInfo->profileIdList[i]) {
    case EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_1_1:
      retVal = emberAfPluginRf4ceProfileRemoteControl11DiscoveryRequestCallback(srcIeeeAddr,
                                                                                nodeCapabilities,
                                                                                vendorInfo,
                                                                                appInfo,
                                                                                searchDevType,
                                                                                rxLinkQuality);
      break;
    case EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_2_0:
      retVal = emberAfPluginRf4ceProfileZrc20DiscoveryRequestCallback(srcIeeeAddr,
                                                                      nodeCapabilities,
                                                                      vendorInfo,
                                                                      appInfo,
                                                                      searchDevType,
                                                                      rxLinkQuality);
      break;
    case EMBER_AF_RF4CE_PROFILE_MSO:
      retVal = emberAfPluginRf4ceProfileMsoDiscoveryRequestCallback(srcIeeeAddr,
                                                                    nodeCapabilities,
                                                                    vendorInfo,
                                                                    appInfo,
                                                                    searchDevType,
                                                                    rxLinkQuality);
      break;
    default:
      // TODO: Handle other profiles.
      break;
    }
  }

  emberAfPopNetworkIndex();
  return retVal;
}

static boolean wantsDiscoveryProfileId(int8u profileId)
{
  int8u i;
  for (i = 0; i < discoveryProfileIdCount; i++) {
    if (discoveryProfileIds[i] == profileId) {
      return TRUE;
    }
  }
  return FALSE;
}

boolean emAfRf4ceDiscoveryResponseHandler(boolean atCapacity,
                                          int8u channel,
                                          EmberPanId panId,
                                          EmberEUI64 srcIeeeAddr,
                                          int8u nodeCapabilities,
                                          const EmberRf4ceVendorInfo *vendorInfo,
                                          const EmberRf4ceApplicationInfo *appInfo,
                                          int8u rxLinkQuality,
                                          int8u discRequestLqi)
{
  boolean retVal = FALSE;
  if (DISCOVERY_IN_PROGRESS()) {
    int8u i;
    int8u profileIdListLength = emberAfRf4ceProfileIdListLength(appInfo->capabilities);

    emberAfPushCallbackNetworkIndex();

    for (i = 0; i < profileIdListLength && !retVal; i++) {
      if (wantsDiscoveryProfileId(appInfo->profileIdList[i])) {
        switch (appInfo->profileIdList[i]) {
        case EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_1_1:
          retVal = emberAfPluginRf4ceProfileRemoteControl11DiscoveryResponseCallback(atCapacity,
                                                                                     channel,
                                                                                     panId,
                                                                                     srcIeeeAddr,
                                                                                     nodeCapabilities,
                                                                                     vendorInfo,
                                                                                     appInfo,
                                                                                     rxLinkQuality,
                                                                                     discRequestLqi);
          break;
        case EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_2_0:
          retVal = emberAfPluginRf4ceProfileZrc20DiscoveryResponseCallback(atCapacity,
                                                                           channel,
                                                                           panId,
                                                                           srcIeeeAddr,
                                                                           nodeCapabilities,
                                                                           vendorInfo,
                                                                           appInfo,
                                                                           rxLinkQuality,
                                                                           discRequestLqi);
          break;
        case EMBER_AF_RF4CE_PROFILE_MSO:
          retVal = emberAfPluginRf4ceProfileMsoDiscoveryResponseCallback(atCapacity,
                                                                         channel,
                                                                         panId,
                                                                         srcIeeeAddr,
                                                                         nodeCapabilities,
                                                                         vendorInfo,
                                                                         appInfo,
                                                                         rxLinkQuality,
                                                                         discRequestLqi);
          break;
        default:
          // TODO: Handle other profiles.
          break;
        }
      }
    }

    emberAfPopNetworkIndex();
  }
  return retVal;
}

void emAfRf4ceDiscoveryCompleteHandler(EmberStatus status)
{
  if (DISCOVERY_IN_PROGRESS()) {
    int8u i;

    // Clear the state (after saving a copy) so that the application is able to
    // perform another discovery right away.
    int8u profileIdCount = discoveryProfileIdCount;
    int8u profileIds[EMBER_RF4CE_APPLICATION_PROFILE_ID_LIST_MAX_LENGTH];
    MEMMOVE(profileIds, discoveryProfileIds, profileIdCount);
    UNSET_DISCOVERY_IN_PROGRESS();

    emberAfPushCallbackNetworkIndex();
    for (i = 0; i < profileIdCount; i++) {
      switch (profileIds[i]) {
      case EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_1_1:
        emberAfPluginRf4ceProfileRemoteControl11DiscoveryCompleteCallback(status);
        break;
      case EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_2_0:
        emberAfPluginRf4ceProfileZrc20DiscoveryCompleteCallback(status);
        break;
      case EMBER_AF_RF4CE_PROFILE_MSO:
        emberAfPluginRf4ceProfileMsoDiscoveryCompleteCallback(status);
        break;
      default:
        // TODO: Handle other profiles.
        break;
      }
    }
    emberAfPopNetworkIndex();
  }
}

EmberStatus emberAfRf4ceEnableAutoDiscoveryResponse(int16u durationMs,
                                                    int8u autoDiscProfileIdListLength,
                                                    int8u *autoDiscProfileIdList)
{
  EmberStatus status = EMBER_INVALID_CALL;
  if (!AUTO_DISCOVERY_IN_PROGRESS()) {
    status = emberAfRf4cePushNetworkIndex();
    if (status == EMBER_SUCCESS) {
      status = emAfRf4ceEnableAutoDiscoveryResponse(durationMs);
      if (status == EMBER_SUCCESS) {
        SET_AUTO_DISCOVERY_IN_PROGRESS(autoDiscProfileIdListLength,
                                       autoDiscProfileIdList);
      }
      emberAfPopNetworkIndex();
    }
  }
  return status;
}

static boolean wantsAutoDiscoveryProfileIds(const EmberRf4ceApplicationInfo *appInfo)
{
  int8u i, j;
  int8u profileIdListLength = emberAfRf4ceProfileIdListLength(appInfo->capabilities);
  for (i = 0; i < profileIdListLength; i++) {
    for (j = 0; i < autoDiscoveryProfileIdCount; j++) {
      if (appInfo->profileIdList[i] == autoDiscoveryProfileIds[j]) {
        return TRUE;
      }
    }
  }
  return FALSE;
}

void emAfRf4ceAutoDiscoveryResponseCompleteHandler(EmberStatus status,
                                                   EmberEUI64 srcIeeeAddr,
                                                   int8u nodeCapabilities,
                                                   const EmberRf4ceVendorInfo *vendorInfo,
                                                   const EmberRf4ceApplicationInfo *appInfo,
                                                   int8u searchDevType)
{
  if (AUTO_DISCOVERY_IN_PROGRESS()) {
    int8u i;

    // Clear the state (after saving a copy) so that the application is able to
    // perform another auto discovery right away.
    int8u profileIdCount = autoDiscoveryProfileIdCount;
    int8u profileIds[EMBER_RF4CE_APPLICATION_PROFILE_ID_LIST_MAX_LENGTH];
    MEMMOVE(profileIds, autoDiscoveryProfileIds, profileIdCount);
    if (status == EMBER_SUCCESS && !wantsAutoDiscoveryProfileIds(appInfo)) {
      status = EMBER_DISCOVERY_TIMEOUT;
    }
    UNSET_AUTO_DISCOVERY_IN_PROGRESS();

    emberAfPushCallbackNetworkIndex();

    for (i = 0; i < profileIdCount; i++) {
      switch (profileIds[i]) {
      case EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_1_1:
        emberAfPluginRf4ceProfileRemoteControl11AutoDiscoveryResponseCompleteCallback(status,
                                                                                      srcIeeeAddr,
                                                                                      nodeCapabilities,
                                                                                      vendorInfo,
                                                                                      appInfo,
                                                                                      searchDevType);
        break;
      case EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_2_0:
        emberAfPluginRf4ceProfileZrc20AutoDiscoveryResponseCompleteCallback(status,
                                                                            srcIeeeAddr,
                                                                            nodeCapabilities,
                                                                            vendorInfo,
                                                                            appInfo,
                                                                            searchDevType);
        break;
      case EMBER_AF_RF4CE_PROFILE_MSO:
        emberAfPluginRf4ceProfileMsoAutoDiscoveryResponseCompleteCallback(status,
                                                                          srcIeeeAddr,
                                                                          nodeCapabilities,
                                                                          vendorInfo,
                                                                          appInfo,
                                                                          searchDevType);
        break;
      default:
        // TODO: Handle other profiles.
        break;
      }
    }

    emberAfPopNetworkIndex();
  }
}

static EmberAfRf4cePairCompleteCallback pairCompletePtr = NULL;

EmberStatus emberAfRf4cePair(int8u channel,
                             EmberPanId panId,
                             EmberEUI64 ieeeAddr,
                             int8u keyExchangeTransferCount,
                             EmberAfRf4cePairCompleteCallback pairCompleteCallback)
{
  EmberStatus status = EMBER_INVALID_CALL;
  if (!PAIRING_IN_PROGRESS()) {
    status = emberAfRf4cePushNetworkIndex();
    if (status == EMBER_SUCCESS) {
      status = emAfRf4cePair(channel,
                             panId,
                             ieeeAddr,
                             keyExchangeTransferCount);
      if (status == EMBER_SUCCESS || status == EMBER_DUPLICATE_ENTRY) {
        SET_PAIRING_IN_PROGRESS();
        pairCompletePtr = pairCompleteCallback;
      }
      emberAfPopNetworkIndex();
    }
  }
  return status;
}

boolean emAfRf4cePairRequestHandler(EmberStatus status,
                                    int8u pairingIndex,
                                    EmberEUI64 srcIeeeAddr,
                                    int8u nodeCapabilities,
                                    const EmberRf4ceVendorInfo *vendorInfo,
                                    const EmberRf4ceApplicationInfo *appInfo,
                                    int8u keyExchangeTransferCount)
{
  boolean retVal = FALSE;

  if (!PAIRING_IN_PROGRESS()) {
    int8u i;
    int8u profileIdListLength = emberAfRf4ceProfileIdListLength(appInfo->capabilities);

    emberAfPushCallbackNetworkIndex();

    for (i = 0; i < profileIdListLength && !retVal; i++) {
      switch (appInfo->profileIdList[i]) {
      case EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_1_1:
        retVal = emberAfPluginRf4ceProfileRemoteControl11PairRequestCallback(status,
                                                                             pairingIndex,
                                                                             srcIeeeAddr,
                                                                             nodeCapabilities,
                                                                             vendorInfo,
                                                                             appInfo,
                                                                             keyExchangeTransferCount);
        break;
      case EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_2_0:
        retVal = emberAfPluginRf4ceProfileZrc20PairRequestCallback(status,
                                                                   pairingIndex,
                                                                   srcIeeeAddr,
                                                                   nodeCapabilities,
                                                                   vendorInfo,
                                                                   appInfo,
                                                                   keyExchangeTransferCount);
        break;
      case EMBER_AF_RF4CE_PROFILE_MSO:
        retVal = emberAfPluginRf4ceProfileMsoPairRequestCallback(status,
                                                                 pairingIndex,
                                                                 srcIeeeAddr,
                                                                 nodeCapabilities,
                                                                 vendorInfo,
                                                                 appInfo,
                                                                 keyExchangeTransferCount);
        break;
      default:
        // TODO: Handle other profiles.
        break;
      }
    }

    if (retVal) {
      SET_PAIRING_IN_PROGRESS();
    }

    emberAfPopNetworkIndex();
  }
  return retVal;
}

void emAfRf4cePairCompleteHandler(EmberStatus status,
                                  int8u pairingIndex,
                                  const EmberRf4ceVendorInfo *vendorInfo,
                                  const EmberRf4ceApplicationInfo *appInfo)
{
  if (PAIRING_IN_PROGRESS()) {
    UNSET_PAIRING_IN_PROGRESS();

    emberAfPushCallbackNetworkIndex();

    // If the application passed a function pointer for the pair complete
    // callback, we only call that one. Otherwise we try to dispatch based on
    // the profile ID list.
    if (pairCompletePtr) {
      // In the pair complete callback the application could invoke another
      // pair call. Therefore, we save the function pointer in a local variable
      // and set the global back to NULL prior to actual callback call.
      EmberAfRf4cePairCompleteCallback callbackPtr = pairCompletePtr;
      pairCompletePtr = NULL;
      (callbackPtr)(status, pairingIndex, vendorInfo, appInfo);
      emberAfPopNetworkIndex();
      return;
    }

    // If the status is not a successful status, the application info is not
    // included in the callback, so we just dispatch to all the plugins.
    // TODO: we might just go back to dispatch to all plugins regardless of
    // status.
    if (status != EMBER_SUCCESS && status != EMBER_DUPLICATE_ENTRY) {
      emberAfPluginRf4ceProfileRemoteControl11PairCompleteCallback(status,
                                                                   pairingIndex,
                                                                   NULL,
                                                                   NULL);
      emberAfPluginRf4ceProfileZrc20PairCompleteCallback(status,
                                                         pairingIndex,
                                                         NULL,
                                                         NULL);
      emberAfPluginRf4ceProfileMsoPairCompleteCallback(status,
                                                       pairingIndex,
                                                       NULL,
                                                       NULL);
    } else {
      int8u profileIdListLength = emberAfRf4ceProfileIdListLength(appInfo->capabilities);
      int8u i;

      for (i = 0; i < profileIdListLength; i++) {
        int8u profileId = appInfo->profileIdList[i];
        switch (profileId) {
        case EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_1_1: {
          emberAfPluginRf4ceProfileRemoteControl11PairCompleteCallback(status,
                                                                       pairingIndex,
                                                                       vendorInfo,
                                                                       appInfo);
          break;
        }
        case EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_2_0: {
          emberAfPluginRf4ceProfileZrc20PairCompleteCallback(status,
                                                             pairingIndex,
                                                             vendorInfo,
                                                             appInfo);
          break;
        }
        case EMBER_AF_RF4CE_PROFILE_MSO: {
          emberAfPluginRf4ceProfileMsoPairCompleteCallback(status,
                                                           pairingIndex,
                                                           vendorInfo,
                                                           appInfo);
          break;
        }
        default:
          // TODO: Handle other profiles.
          break;
        }
      }
    }

    emberAfPopNetworkIndex();
  }
}

EmberStatus emberAfRf4ceSetPairingTableEntry(int8u pairingIndex,
                                             EmberRf4cePairingTableEntry *entry)
{
  EmberStatus status = emberAfRf4cePushNetworkIndex();
  if (status == EMBER_SUCCESS) {
    status = emAfRf4ceSetPairingTableEntry(pairingIndex, entry);
    emberAfPopNetworkIndex();
  }
  return status;
}

EmberStatus emberAfRf4ceGetPairingTableEntry(int8u pairingIndex,
                                             EmberRf4cePairingTableEntry *entry)
{
  EmberStatus status = emberAfRf4cePushNetworkIndex();
  if (status == EMBER_SUCCESS) {
    status = emAfRf4ceGetPairingTableEntry(pairingIndex, entry);
    emberAfPopNetworkIndex();
  }
  return status;
}

EmberStatus emberAfRf4ceSetApplicationInfo(EmberRf4ceApplicationInfo *appInfo)
{
  EmberStatus status = emberAfRf4cePushNetworkIndex();
  if (status == EMBER_SUCCESS) {
    status = emAfRf4ceSetApplicationInfo(appInfo);
    emberAfPopNetworkIndex();
  }
  return status;
}

EmberStatus emberAfRf4ceGetApplicationInfo(EmberRf4ceApplicationInfo *appInfo)
{
  EmberStatus status = emberAfRf4cePushNetworkIndex();
  if (status == EMBER_SUCCESS) {
    status = emAfRf4ceGetApplicationInfo(appInfo);
    emberAfPopNetworkIndex();
  }
  return status;
}

EmberStatus emberAfRf4ceKeyUpdate(int8u pairingIndex, EmberKeyData *key)
{
  EmberStatus status = emberAfRf4cePushNetworkIndex();
  if (status == EMBER_SUCCESS) {
    status = emAfRf4ceKeyUpdate(pairingIndex, key);
    emberAfPopNetworkIndex();
  }
  return status;
}

EmberStatus emberAfRf4ceSetDiscoveryLqiThreshold(int8u threshold)
{
  EmberStatus status = emberAfRf4cePushNetworkIndex();
  if (status == EMBER_SUCCESS) {
    status = emAfRf4ceSetDiscoveryLqiThreshold(threshold);
    emberAfPopNetworkIndex();
  }
  return status;
}

int8u emberAfRf4ceGetBaseChannel(void)
{
  EmberStatus status = emberAfRf4cePushNetworkIndex();
  int8u channel = 0xFF;

  if (status == EMBER_SUCCESS) {
    channel =  emAfRf4ceGetBaseChannel();
    emberAfPopNetworkIndex();
  }
  return channel;
}

EmberStatus emberAfRf4ceSendExtended(int8u pairingIndex,
                                     int8u profileId,
                                     int16u vendorId,
                                     EmberRf4ceTxOption txOptions,
                                     int8u *message,
                                     int8u messageLength,
                                     int8u *messageTag)
{
  EmberStatus status;
  int8u thisMessageTag;

  thisMessageTag = nextMessageTag++ & EMBER_AF_RF4CE_MESSAGE_TAG_MASK;
  if (messageTag != NULL) {
    *messageTag = thisMessageTag;
  }

  status = emberAfRf4cePushNetworkIndex();
  if (status != EMBER_SUCCESS) {
    return status;
  }

  status = emAfRf4ceSend(pairingIndex,
                         profileId,
                         vendorId,
                         txOptions,
                         thisMessageTag,
                         messageLength,
                         message);

  emberAfPopNetworkIndex();
  return status;
}

EmberStatus emberAfRf4ceSend(int8u pairingIndex,
                             int8u profileId,
                             int8u *message,
                             int8u messageLength,
                             int8u *messageTag)
{
  EmberRf4ceTxOption txOptions;
  EmberStatus status = emberAfRf4ceGetDefaultTxOptions(pairingIndex,
                                                       &txOptions);

  if (status != EMBER_SUCCESS) {
    return status;
  }

  return emberAfRf4ceSendExtended(pairingIndex,
                                  profileId,
                                  EMBER_RF4CE_NULL_VENDOR_ID,
                                  txOptions,
                                  message,
                                  messageLength,
                                  messageTag);
}

EmberStatus emberAfRf4ceSendVendorSpecific(int8u pairingIndex,
                                           int8u profileId,
                                           int16u vendorId,
                                           int8u *message,
                                           int8u messageLength,
                                           int8u *messageTag)
{
  EmberRf4ceTxOption txOptions;
  EmberStatus status = emberAfRf4ceGetDefaultTxOptions(pairingIndex,
                                                       &txOptions);

  if (status != EMBER_SUCCESS) {
    return status;
  }

  return emberAfRf4ceSendExtended(pairingIndex,
                                  profileId,
                                  vendorId,
                                  (txOptions
                                   | EMBER_RF4CE_TX_OPTIONS_VENDOR_SPECIFIC_BIT),
                                   message,
                                   messageLength,
                                   messageTag);
}

EmberStatus emberAfRf4ceGetDefaultTxOptions(int8u pairingIndex,
                                            EmberRf4ceTxOption *txOptions)
{
  EmberStatus status = EMBER_SUCCESS;

  if (pairingIndex == EMBER_RF4CE_PAIRING_TABLE_BROADCAST_INDEX) {
    *txOptions = EMBER_RF4CE_TX_OPTIONS_BROADCAST_BIT;
  } else {
    EmberRf4cePairingTableEntry entry;

    status = emberAfRf4ceGetPairingTableEntry(pairingIndex, &entry);
    if (status == EMBER_SUCCESS) {
      *txOptions = EMBER_RF4CE_TX_OPTIONS_ACK_REQUESTED_BIT;

      if (emberAfRf4cePairingTableEntryHasSecurity(&entry)
          && emberAfRf4cePairingTableEntryHasLinkKey(&entry)) {
        *txOptions |= EMBER_RF4CE_TX_OPTIONS_SECURITY_ENABLED_BIT;
      }

#ifdef EMBER_AF_RF4CE_NODE_TYPE_TARGET
      if (emberAfRf4cePairingTableEntryHasChannelNormalization(&entry)) {
        *txOptions |= EMBER_RF4CE_TX_OPTIONS_CHANNEL_DESIGNATOR_BIT;
      }
#endif

      // TODO: Use long addressing if there is enough room in the payload.
      //*txOptions |= EMBER_RF4CE_TX_OPTIONS_USE_IEEE_ADDRESS_BIT;
    }
  }

  return status;
}

void emAfRf4ceMessageSentHandler(EmberStatus status,
                                 int8u pairingIndex,
                                 int8u txOptions,
                                 int8u profileId,
                                 int16u vendorId,
                                 int8u messageTag,
                                 int8u messageLength,
                                 const int8u *message)
{
  emberAfPushCallbackNetworkIndex();
  SET_PAIRING_INDEX(pairingIndex);

  // GDP 2.0 commands are tricky since they can be sent with the GDP 2.0 profile
  // ID or any GDP 2.0 based profile ID. We want to use the common GDP 2.0 code
  // for the commands that carry the GDP profile ID and other GDP-based profile
  // IDs. Therefore, first we try to dispatch to GDP callback. This callback is
  // expected to return TRUE if the passed message is a GDP 2.0 command, FALSE
  // otherwise.
  if (!emberAfPluginRf4ceProfileGdpMessageSentCallback(pairingIndex,
                                                       profileId,
                                                       vendorId,
                                                       messageTag,
                                                       message,
                                                       messageLength,
                                                       status)) {
    switch (profileId) {
    case EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_1_1:
      emberAfPluginRf4ceProfileRemoteControl11MessageSentCallback(pairingIndex,
                                                                  vendorId,
                                                                  messageTag,
                                                                  message,
                                                                  messageLength,
                                                                  status);
      break;
    case EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_2_0:
      emberAfPluginRf4ceProfileZrc20MessageSentCallback(pairingIndex,
                                                        vendorId,
                                                        messageTag,
                                                        message,
                                                        messageLength,
                                                        status);
      break;
    case EMBER_AF_RF4CE_PROFILE_MSO:
      emberAfPluginRf4ceProfileMsoMessageSentCallback(pairingIndex,
                                                      vendorId,
                                                      messageTag,
                                                      message,
                                                      messageLength,
                                                      status);
      break;
    default:
      // TODO: Handle other profiles.
      break;
    }
  }

  emberAfPluginRf4ceProfileMessageSentCallback(pairingIndex,
                                               profileId,
                                               vendorId,
                                               messageTag,
                                               message,
                                               messageLength,
                                               status);

  UNSET_PAIRING_INDEX();
  emberAfPopNetworkIndex();
}

void emAfRf4ceIncomingMessageHandler(int8u pairingIndex,
                                     int8u profileId,
                                     int16u vendorId,
                                     EmberRf4ceTxOption txOptions,
                                     int8u messageLength,
                                     const int8u *message)
{
  emberAfPushCallbackNetworkIndex();
  SET_PAIRING_INDEX(pairingIndex);

  // GDP 2.0 commands are tricky since they can be sent with the GDP 2.0 profile
  // ID or any GDP 2.0 based profile ID. We want to use the common GDP 2.0 code
  // for the commands that carry the GDP profile ID and other GDP-based profile
  // IDs. Therefore, first we try to dispatch to the GDP callback.
  // This callback is expected to return TRUE if the passed message was
  // processed by the GDP code, FALSE otherwise.
  if (!emberAfPluginRf4ceProfileGdpIncomingMessageCallback(pairingIndex,
                                                           profileId,
                                                           vendorId,
                                                           txOptions,
                                                           message,
                                                           messageLength)) {
    // Dispatch the incoming message based on the profile ID.
    switch (profileId) {
    case EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_1_1:
      emberAfPluginRf4ceProfileRemoteControl11IncomingMessageCallback(pairingIndex,
                                                                      vendorId,
                                                                      txOptions,
                                                                      message,
                                                                      messageLength);
      break;
    case EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_2_0:
      emberAfPluginRf4ceProfileZrc20IncomingMessageCallback(pairingIndex,
                                                            vendorId,
                                                            txOptions,
                                                            message,
                                                            messageLength);
      break;
    case EMBER_AF_RF4CE_PROFILE_MSO:
      emberAfPluginRf4ceProfileMsoIncomingMessageCallback(pairingIndex,
                                                          vendorId,
                                                          txOptions,
                                                          message,
                                                          messageLength);
      break;
    default:
      // TODO: Handle other profiles.
      break;
    }
  }

  emberAfPluginRf4ceProfileIncomingMessageCallback(pairingIndex,
                                                   profileId,
                                                   vendorId,
                                                   txOptions,
                                                   message,
                                                   messageLength);

  UNSET_PAIRING_INDEX();
  emberAfPopNetworkIndex();
}

int8u emberAfRf4ceGetPairingIndex(void)
{
  return currentPairingIndex;
}

EmberStatus emberAfRf4ceUnpair(int8u pairingIndex)
{
  EmberStatus status = EMBER_INVALID_CALL;
  if (!UNPAIRING_IN_PROGRESS()) {
    status = emberAfRf4cePushNetworkIndex();
    if (status == EMBER_SUCCESS) {
      status = emAfRf4ceUnpair(pairingIndex);
      if (status == EMBER_SUCCESS) {
        SET_UNPAIRING_IN_PROGRESS();
      }
      emberAfPopNetworkIndex();
    }
  }
  return status;
}

void emAfRf4ceUnpairHandler(int8u pairingIndex)
{
  emberAfPushCallbackNetworkIndex();
  // TODO
  emberAfPopNetworkIndex();
}

void emAfRf4ceUnpairCompleteHandler(int8u pairingIndex)
{
  if (UNPAIRING_IN_PROGRESS()) {
    UNSET_UNPAIRING_IN_PROGRESS();
    emberAfPushCallbackNetworkIndex();
    // TODO
    emberAfPopNetworkIndex();
  }
}

boolean emberAfRf4ceIsDeviceTypeSupported(const EmberRf4ceApplicationInfo *appInfo,
                                          EmberAfRf4ceDeviceType deviceType)
{
  int8u deviceTypeListLength = emberAfRf4ceDeviceTypeListLength(appInfo->capabilities);
  int8u i;

  // The wildcard device type matches everything.
  if (deviceType == EMBER_AF_RF4CE_DEVICE_TYPE_WILDCARD) {
    return TRUE;
  }

  for (i = 0; i < deviceTypeListLength; i++) {
    if (appInfo->deviceTypeList[i] == deviceType) {
      return TRUE;
    }
  }

  return FALSE;
}

boolean emberAfRf4ceIsProfileSupported(const EmberRf4ceApplicationInfo *appInfo,
                                       EmberAfRf4ceProfileId profileId)
{
  int8u profileIdListLength = emberAfRf4ceProfileIdListLength(appInfo->capabilities);
  int8u i;

  for (i = 0; i < profileIdListLength; i++) {
    if (appInfo->profileIdList[i] == profileId) {
      return TRUE;
    }
  }

  return FALSE;
}
